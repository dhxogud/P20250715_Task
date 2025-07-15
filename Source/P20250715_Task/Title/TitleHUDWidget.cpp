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
	SignupButtton = Cast<UButton>(GetWidgetFromName("Signup_Button"));
	LoginButtton = Cast<UButton>(GetWidgetFromName("Login_Button"));

	if (SignupButtton)
	{
		SignupButtton->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnClickLoginButton);
	}

	if (LoginButtton)
	{
		LoginButtton->OnClicked.AddDynamic(this, &UTitleHUDWidget::OnClickLoginButton);
	}
}

void UTitleHUDWidget::OnClickSignupButtton()
{
	ATitlePC* PC = Cast< ATitlePC>(GetOwningPlayer());
	if (PC)
	{
		PC->Signup();
	}
}

void UTitleHUDWidget::OnClickLoginButton()
{
	ATitlePC* PC = Cast< ATitlePC>(GetOwningPlayer());
	if (PC)
	{
		PC->Login(UserIdText->GetText(), UserPasswordText->GetText());
	}
}


