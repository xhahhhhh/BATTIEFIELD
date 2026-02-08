// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "BATTLEFIELD/PlayerState/BasePlayerState.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		ABasePlayerState* BPState = NewPlayer->GetPlayerState<ABasePlayerState>();
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	ABasePlayerState* BPState = Exiting->GetPlayerState<ABasePlayerState>();
	if (BGameState && BPState)
	{
		if (BGameState->RedTeam.Contains(BPState))
		{
			BGameState->RedTeam.Remove(BPState);
		}
		else if (BGameState->BlueTeam.Contains(BPState))
		{
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PState : BGameState->PlayerArray)
		{
			ABasePlayerState* BPState = Cast<ABasePlayerState>(PState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ABasePlayerState* AttackerPState = Attacker->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* VictimPState = Victim->GetPlayerState<ABasePlayerState>();
	if (AttackerPState == nullptr || VictimPState == nullptr)return BaseDamage;
	if (VictimPState == AttackerPState)return BaseDamage;
	if (AttackerPState->GetTeam() == VictimPState->GetTeam()) return  0.f;
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
	ABasePlayerController* VictimController, ABasePlayerController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
	
	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	ABasePlayerState* AttackerPlayerState =AttackerController? Cast<ABasePlayerState>(AttackerController->PlayerState) : nullptr;
	if (BGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BGameState->BlueTeamScores();
		}
		if(AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BGameState->RedTeamScores();
		}
	}
}
