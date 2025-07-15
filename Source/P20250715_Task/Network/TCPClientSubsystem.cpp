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

    // ���� ����ý��� �ʱ�ȭ
    SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    if (!SocketSubsystem)
    {
        LogMessage(TEXT("Failed to get socket subsystem"));
        return;
    }

    LogMessage(TEXT("Echo Client Subsystem initialized"));

    // Ÿ�̸� ���� (0.1�ʸ��� �޽��� üũ)
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TickTimerHandle, this, &UTCPClientSubsystem::OnTick, 0.1f, true);
    }
}

void UTCPClientSubsystem::Deinitialize()
{
    // Ÿ�̸� ����
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
        // ���� ���� Ȯ��
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

    // ���� ����
    ClientSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("EchoClient"), false);

    if (!ClientSocket)
    {
        LogMessage(TEXT("Failed to create socket"));
        return false;
    }

    // ���� �ּ� ����
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

    // ������ ����
    bool bConnected = ClientSocket->Connect(*ServerAddr);

    if (bConnected)
    {
        ServerIP = ServerAddress;
        ServerPort = Port;
        bIsConnected = true;

        // ����ŷ ��� ����
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

    // �޽����� UTF-8�� ��ȯ
    FTCHARToUTF8 UTF8Message(*Message);

    // �޽��� ���� + �޽��� ���·� ����
    int32 MessageLength = UTF8Message.Length();
    //char Buffer[1024];
    TArray<uint8> SendBuffer;

    // �޽��� ���̸� 4����Ʈ�� ���� (��Ʈ��ũ ����Ʈ ����)
    uint32 NetworkMessageLength = ByteSwap(MessageLength);
    SendBuffer.Append(reinterpret_cast<const uint8*>(&NetworkMessageLength), sizeof(uint32));

    // �޽��� ���� �߰�
    SendBuffer.Append(reinterpret_cast<const uint8*>(UTF8Message.Get()), MessageLength);

    // ������ ����
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

    // ������ �����Ͱ� �ִ��� Ȯ��
    uint32 PendingDataSize = 0;
    if (!ClientSocket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
        return;

    // �޽��� ���� �б� (4����Ʈ)
    uint32 MessageLength = 0;
    int32 BytesRead = 0;

    if (!ClientSocket->Recv(reinterpret_cast<uint8*>(&MessageLength), sizeof(uint32), BytesRead))
        return;

    if (BytesRead != sizeof(uint32))
        return;

    // ��Ʈ��ũ ����Ʈ ������ ȣ��Ʈ ����Ʈ ������ ��ȯ
    MessageLength = ByteSwap(MessageLength);

    LogMessage(FString::Printf(TEXT("MessgeSize :%d"), MessageLength));

    // �޽����� �ʹ� ũ�� ����
    if (MessageLength > 1024 * 1024) // 1MB ����
    {
        LogMessage(TEXT("Received message too large, ignoring"));
        return;
    }

    // �޽��� ���� �б�
    TArray<uint8> MessageBuffer;
    MessageBuffer.SetNum(MessageLength + 1);

    LogMessage(FString::Printf(TEXT("MessgeSize :%d"), MessageBuffer.Num()));

    if (!ClientSocket->Recv(MessageBuffer.GetData(), MessageLength, BytesRead))
        return;

    if (BytesRead != MessageLength)
        return;

    // UTF-8���� FString���� ��ȯ
    FString ReceivedMessage = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(MessageBuffer.GetData())));

    ProcessReceivedMessage(ReceivedMessage);
}

void UTCPClientSubsystem::ProcessReceivedMessage(const FString& Message)
{
    LogMessage(FString::Printf(TEXT("Received echo: %s"), *Message));

    // �������Ʈ �̺�Ʈ �߻�
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
