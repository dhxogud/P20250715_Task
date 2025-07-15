// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleHUDWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "TitlePC.h"

void UTitleHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();


	UserIdText = Cast< UEditableTextBox>(GetWidgetFromName("IdInputBox"));
	UserPasswordText = Cast< UEditableTextBox>(GetWidgetFromName("PasswordInputBox"));
	SignUpButtton = Cast<UButton>(GetWidgetFromName("Signup_Button"));
	LoginButtton = Cast<UButton>(GetWidgetFromName("Login_Button"));

	if (SignUpButtton)
	{
		SignUpButtton->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnClickLoginButton);
	}

	if (LoginButtton)
	{
		LoginButtton->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnClickLoginButton);
	}
}

void UTitleHUDWidget::OnClickSignUpButtton()
{
	ATitlePC* PC = Cast< ATitlePC>(GetOwningPlayer());
	if (PC)
	{

	}
}

void UTitleHUDWidget::OnClickLoginButton()
{
	ATitlePC* PC = Cast< ATitlePC>(GetOwningPlayer());
	if (PC)
	{

	}
}


