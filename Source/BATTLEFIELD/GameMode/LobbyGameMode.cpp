// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiPlayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiPlayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
		check(Subsystem);

		if (NumberOfPlayers == Subsystem->GetDesiredNumPublicConnections())
		{
			UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;

				FString MatchType = Subsystem->GetDesiredMatchType();
				if (MatchType == "FreeForAll")
				{
					World->ServerTravel(FString("/Game/Maps/Map1?listen"));
				}
				else if (MatchType == "Teams")
				{
					World->ServerTravel(FString("/Game/Maps/Map1?listen"));
				}
				else if (MatchType == "CaptureTheFlag")
				{
					World->ServerTravel(FString("/Game/Maps/Map1?listen"));
				}
			}
		}
	}
}
