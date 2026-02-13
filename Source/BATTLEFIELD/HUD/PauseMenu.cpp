// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenu.h"

#include "MultiPlayerSessionsSubsystem.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

bool UPauseMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	return true;
}

void UPauseMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiPlayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
		if (MultiPlayerSessionsSubsystem)
		{
			MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.AddDynamic(
				this, &UPauseMenu::OnDetroySession);
		}
	}
}

void UPauseMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}
	if (MultiPlayerSessionsSubsystem && MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.IsBound())
	{
		MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.RemoveDynamic(
			this, &UPauseMenu::OnDetroySession);
	}
}


void UPauseMenu::OnDetroySession(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode)
		{
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UPauseMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FirstPlayerController->GetPawn());
			if (BaseCharacter)
			{
				BaseCharacter->OnLeftGame.AddDynamic(this,&UPauseMenu::OnPlayerLeftGame);
				BaseCharacter->ServerLeaveGame();
			}
			else
			{
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UPauseMenu::OnPlayerLeftGame()
{
	if (MultiPlayerSessionsSubsystem)
	{
		MultiPlayerSessionsSubsystem->DestroySession();
	}
}
