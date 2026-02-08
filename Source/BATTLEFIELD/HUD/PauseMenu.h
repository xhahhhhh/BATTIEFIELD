// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenu.generated.h"

class UButton;
class UTextBlock;
class UMultiPlayerSessionsSubsystem;
class APlayerController;
UCLASS()
class BATTLEFIELD_API UPauseMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void MenuSetup();
	void MenuTearDown();
	
protected:
	virtual bool Initialize() override;
	
	UFUNCTION()
	void OnDetroySession(bool bWasSuccessful);
	
	UFUNCTION()
	void OnPlayerLeftGame();
private:
	UPROPERTY(meta = (BindWidget))
	UButton* ReturnButton;
	
	UFUNCTION()
	void ReturnButtonClicked();
	
	UPROPERTY()
	UMultiPlayerSessionsSubsystem* MultiPlayerSessionsSubsystem;
	
	UPROPERTY()
	APlayerController* PlayerController;
};
