// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerGameMode.h"
#include "../Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "../PlayerController/BasePlayerController.h"
#include "../PlayerState/BasePlayerState.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "GameFramework/PlayerStart.h"

APlayerGameMode::APlayerGameMode()
{
	bDelayedStart = true;
}

void APlayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void APlayerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void APlayerGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABasePlayerController* BasePlayer = Cast<ABasePlayerController>(*It);
		if (BasePlayer)
		{
			BasePlayer->OnMatchStateSet(MatchState,bTeamMatch);
		}
	}
}

float APlayerGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

void APlayerGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
                                       ABasePlayerController* VictimController,
                                       ABasePlayerController* AttackerController)
{
	ABasePlayerState* AttackerPlayerState = AttackerController
		                                        ? Cast<ABasePlayerState>(AttackerController->PlayerState)
		                                        : nullptr;
	ABasePlayerState* VictimPlayerState = VictimController
		                                      ? Cast<ABasePlayerState>(VictimController->PlayerState)
		                                      : nullptr;

	ABaseGameState* BaseGameState = GetGameState<ABaseGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BaseGameState)
	{
		TArray<ABasePlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : BaseGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.AddUnique(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		BaseGameState->UpdateTopScore(AttackerPlayerState);
		if (BaseGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABaseCharacter* Leader = Cast<ABaseCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!BaseGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABaseCharacter* Loser = Cast<ABaseCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim(false);
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABasePlayerController* BasePlayerController = Cast<ABasePlayerController>(*It);
		if (BasePlayerController && AttackerPlayerState && VictimPlayerState)
		{
			BasePlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void APlayerGameMode::RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void APlayerGameMode::PlayerLeftGame(ABasePlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	ABaseGameState* BaseGameState = GetGameState<ABaseGameState>();
	if (BaseGameState && BaseGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BaseGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	ABaseCharacter* CharacterLeaving = Cast<ABaseCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}


