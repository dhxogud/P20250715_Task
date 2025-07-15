// Fill out your copyright notice in the Description page of Project Settings.


#include "TCPClientSubsystem.h"

#include "Engine/Engine.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "Serialization/ArrayReader.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/UnrealMemory.h"
#include "Misc/ByteSwap.h"

UTCPClientSubsystem::UTCPClientSubsystem()
{
    ClientSocket = nullptr;
    SocketSubsystem = nullptr;
    bIsConnected = false;
    ServerPort = 0;
}

void UTCPClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 소켓 서브시스템 초기화
    SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    if (!SocketSubsystem)
    {
        LogMessage(TEXT("Failed to get socket subsystem"));
        return;
    }

    LogMessage(TEXT("Echo Client Subsystem initialized"));

    // 타이머 설정 (0.1초마다 메시지 체크)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TickTimerHandle, this, &UTCPClientSubsystem::OnTick, 0.1f, true);
    }
}

void UTCPClientSubsystem::Deinitialize()
{
    // 타이머 정리
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }

    DisconnectFromServer();
    Super::Deinitialize();
}

bool UTCPClientSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UTCPClientSubsystem::OnTick()
{
    if (bIsConnected && ClientSocket)
    {
        // 연결 상태 확인
        if (ClientSocket->GetConnectionState() != SCS_Connected)
        {
            LogMessage(TEXT("Connection lost"));
            CleanupSocket();
            NotifyConnectionStatusChanged(false);
            return;
        }

        ReceiveMessages();
    }
}

bool UTCPClientSubsystem::ConnectToServer(const FString& ServerAddress, int32 Port)
{
    if (bIsConnected)
    {
        LogMessage(TEXT("Already connected to server"));
        return false;
    }

    if (!SocketSubsystem)
    {
        LogMessage(TEXT("Socket subsystem not available"));
        return false;
    }

    // 소켓 생성
    ClientSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("EchoClient"), false);

    if (!ClientSocket)
    {
        LogMessage(TEXT("Failed to create socket"));
        return false;
    }

    // 서버 주소 설정
    FIPv4Address IPv4Address;
    if (!FIPv4Address::Parse(ServerAddress, IPv4Address))
    {
        LogMessage(FString::Printf(TEXT("Invalid IP address: %s"), *ServerAddress));
        CleanupSocket();
        return false;
    }

    TSharedRef<FInternetAddr> ServerAddr = SocketSubsystem->CreateInternetAddr();
    ServerAddr->SetIp(IPv4Address.Value);
    ServerAddr->SetPort(Port);

    // 서버에 연결
    bool bConnected = ClientSocket->Connect(*ServerAddr);

    if (bConnected)
    {
        ServerIP = ServerAddress;
        ServerPort = Port;
        bIsConnected = true;

        // 논블로킹 모드 설정
        ClientSocket->SetNonBlocking(true);

        LogMessage(FString::Printf(TEXT("Connected to server %s:%d"), *ServerAddress, Port));
        NotifyConnectionStatusChanged(true);
        return true;
    }
    else
    {
        LogMessage(FString::Printf(TEXT("Failed to connect to server %s:%d"), *ServerAddress, Port));
        CleanupSocket();
        return false;
    }
}

bool UTCPClientSubsystem::SendMessage(const FString& Message)
{
    if (!bIsConnected || !ClientSocket)
    {
        LogMessage(TEXT("Not connected to server"));
        return false;
    }

    // 메시지를 UTF-8로 변환
    FTCHARToUTF8 UTF8Message(*Message);

    // 메시지 길이 + 메시지 형태로 전송
    int32 MessageLength = UTF8Message.Length();
    //char Buffer[1024];
    TArray<uint8> SendBuffer;

    // 메시지 길이를 4바이트로 전송 (네트워크 바이트 순서)
    uint32 NetworkMessageLength = ByteSwap(MessageLength);
    SendBuffer.Append(reinterpret_cast<const uint8*>(&NetworkMessageLength), sizeof(uint32));

    // 메시지 내용 추가
    SendBuffer.Append(reinterpret_cast<const uint8*>(UTF8Message.Get()), MessageLength);

    // 데이터 전송
    int32 BytesSent = 0;
    bool bSent = ClientSocket->Send(SendBuffer.GetData(), SendBuffer.Num(), BytesSent);

    if (bSent && BytesSent == SendBuffer.Num())
    {
        LogMessage(FString::Printf(TEXT("Sent message: %s"), *Message));
        return true;
    }
    else
    {
        LogMessage(FString::Printf(TEXT("Failed to send message: %s"), *Message));
        return false;
    }
}

void UTCPClientSubsystem::DisconnectFromServer()
{
    if (bIsConnected)
    {
        NotifyConnectionStatusChanged(false);
    }

    CleanupSocket();

    ServerIP.Empty();
    ServerPort = 0;

    LogMessage(TEXT("Disconnected from server"));
}

bool UTCPClientSubsystem::IsConnected() const
{
    return bIsConnected && ClientSocket && ClientSocket->GetConnectionState() == SCS_Connected;
}

FString UTCPClientSubsystem::GetServerInfo() const
{
    if (bIsConnected)
    {
        return FString::Printf(TEXT("%s:%d"), *ServerIP, ServerPort);
    }
    return TEXT("Not connected");
}

void UTCPClientSubsystem::CheckForMessages()
{
    if (bIsConnected && ClientSocket)
    {
        ReceiveMessages();
    }
}

void UTCPClientSubsystem::ReceiveMessages()
{
    if (!ClientSocket)
        return;

    // 수신할 데이터가 있는지 확인
    uint32 PendingDataSize = 0;
    if (!ClientSocket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
        return;

    // 메시지 길이 읽기 (4바이트)
    uint32 MessageLength = 0;
    int32 BytesRead = 0;

    if (!ClientSocket->Recv(reinterpret_cast<uint8*>(&MessageLength), sizeof(uint32), BytesRead))
        return;

    if (BytesRead != sizeof(uint32))
        return;

    // 네트워크 바이트 순서를 호스트 바이트 순서로 변환
    MessageLength = ByteSwap(MessageLength);

    LogMessage(FString::Printf(TEXT("MessgeSize :%d"), MessageLength));

    // 메시지가 너무 크면 무시
    if (MessageLength > 1024 * 1024) // 1MB 제한
    {
        LogMessage(TEXT("Received message too large, ignoring"));
        return;
    }

    // 메시지 내용 읽기
    TArray<uint8> MessageBuffer;
    MessageBuffer.SetNum(MessageLength + 1);

    LogMessage(FString::Printf(TEXT("MessgeSize :%d"), MessageBuffer.Num()));

    if (!ClientSocket->Recv(MessageBuffer.GetData(), MessageLength, BytesRead))
        return;

    if (BytesRead != MessageLength)
        return;

    // UTF-8에서 FString으로 변환
    FString ReceivedMessage = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(MessageBuffer.GetData())));

    ProcessReceivedMessage(ReceivedMessage);
}

void UTCPClientSubsystem::ProcessReceivedMessage(const FString& Message)
{
    LogMessage(FString::Printf(TEXT("Received echo: %s"), *Message));

    // 블루프린트 이벤트 발생
    OnMessageReceived.Broadcast(Message);
}

void UTCPClientSubsystem::NotifyConnectionStatusChanged(bool bNewConnectionStatus)
{
    bIsConnected = bNewConnectionStatus;
    OnConnectionStatusChanged.Broadcast(bNewConnectionStatus);
}

void UTCPClientSubsystem::CleanupSocket()
{
    if (ClientSocket)
    {
        ClientSocket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(ClientSocket);
        }
        ClientSocket = nullptr;
    }

    bIsConnected = false;
}

void UTCPClientSubsystem::LogMessage(const FString& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("EchoClientSubsystem: %s"), *Message);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
            FString::Printf(TEXT("EchoClientSubsystem: %s"), *Message));
    }
}
