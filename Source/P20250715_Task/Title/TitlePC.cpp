// Fill out your copyright notice in the Description page of Project Settings.


#include "TitlePC.h"
#include "Kismet/GameplayStatics.h"
#include "../Network/TCPClientSubsystem.h"

void ATitlePC::Signup()
{
	UTCPClientSubsystem* NetworkSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UTCPClientSubsystem>();

	if (NetworkSubsystem)
	{

	}
}

void ATitlePC::Login(FText InUserIdText, FText InUserPasswordText)
{
	UTCPClientSubsystem* NetworkSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UTCPClientSubsystem>();

	if (NetworkSubsystem)
	{

	}
	//UGameplayStatics::OpenLevel(GetWorld(), FName(ServerIP.ToString()), true, TEXT("Hello"));
}
