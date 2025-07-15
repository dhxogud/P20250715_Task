// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "sockets.h"
#include "SocketSubsystem.h"
#include "TCPClientSubsystem.generated.h"

/**
 * 
 */


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, bool, bIsConnected);

UCLASS()

class P20250715_TASK_API UTCPClientSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
    UTCPClientSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    // 서버에 연결
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    bool ConnectToServer(const FString& ServerAddress, int32 Port);

    // 서버로 메시지 전송
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    bool SendMessage(const FString& Message);

    // 서버 연결 해제
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    void DisconnectFromServer();

    // 연결 상태 확인
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    bool IsConnected() const;

    // 서버 정보 가져오기
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    FString GetServerInfo() const;

    // 이벤트 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "TCP Client")
    FOnMessageReceived OnMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "TCP Client")
    FOnConnectionStatusChanged OnConnectionStatusChanged;

    // 수동으로 메시지 수신 체크 (Tick 대신 사용 가능)
    UFUNCTION(BlueprintCallable, Category = "TCP Client")
    void CheckForMessages();

protected:
    // 타이머 핸들러
    void OnTick();
    
private:
    // 소켓 관련
    FSocket* ClientSocket;
    ISocketSubsystem* SocketSubsystem;

    // 서버 정보
    FString ServerIP;
    int32 ServerPort;

    // 연결 상태
    bool bIsConnected;

    // 타이머 핸들
    FTimerHandle TickTimerHandle;

    // 메시지 수신 처리
    void ReceiveMessages();

    // 수신된 메시지를 처리
    void ProcessReceivedMessage(const FString& Message);

    // 연결 상태 변경 알림
    void NotifyConnectionStatusChanged(bool bNewConnectionStatus);

    // 로그 출력
    void LogMessage(const FString& Message);

    // 소켓 정리
    void CleanupSocket();

};
