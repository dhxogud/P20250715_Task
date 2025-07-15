// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleHUDWidget.generated.h"

/**
 * 
 */
class UEditableTextBox;
class UButton;

UCLASS()
class P20250715_TASK_API UTitleHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (WidgetBinding), VisibleAnywhere, Category = "Components", BlueprintReadOnly)
	TObjectPtr<UEditableTextBox> UserIdText;

	UPROPERTY(meta = (WidgetBinding), Category = "Components", BlueprintReadOnly)
	TObjectPtr<UEditableTextBox> UserPasswordText;

	UPROPERTY(meta = (WidgetBinding), VisibleAnywhere, Category = "Components", BlueprintReadOnly)
	TObjectPtr<UButton> LoginButtton;

	UPROPERTY(meta = (WidgetBinding), VisibleAnywhere, Category = "Components", BlueprintReadOnly)
	TObjectPtr<UButton> SignupButtton;

	UFUNCTION()
	void OnClickSignupButtton();

	UFUNCTION()
	void OnClickLoginButton();

	
};
