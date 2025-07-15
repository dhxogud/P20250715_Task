// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePC.generated.h"

/**
 * 
 */
UCLASS()
class P20250715_TASK_API ATitlePC : public APlayerController
{
	GENERATED_BODY()
public:
	void Signup();
	void Login(FText InUserIdText, FText InUserPasswordText);
};
