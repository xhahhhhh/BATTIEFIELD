// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerState.h"

#include <filesystem>

#include "../Character/BaseCharacter.h"
#include "../PlayerController/BasePlayerController.h"
#include "Net/UnrealNetwork.h"

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABasePlayerState,Defeats);
	DOREPLIFETIME(ABasePlayerState,Team);
}

void ABasePlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	Character = Character == nullptr?Cast<ABaseCharacter>(GetPawn()):Character;
	if (Character)
	{
		Controller = Controller == nullptr?Cast<ABasePlayerController>(Character->Controller):Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABasePlayerState::OnRep_Defeats()
{
	Character = Character == nullptr?Cast<ABaseCharacter>(GetPawn()):Character;
	if (Character)
	{
		Controller = Controller == nullptr?Cast<ABasePlayerController>(Character->Controller):Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABasePlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = Character == nullptr?Cast<ABaseCharacter>(GetPawn()):Character;
	if (Character)
	{
		Controller = Controller == nullptr?Cast<ABasePlayerController>(Character->Controller):Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABasePlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr?Cast<ABaseCharacter>(GetPawn()):Character;
	if (Character)
	{
		Controller = Controller == nullptr?Cast<ABasePlayerController>(Character->Controller):Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABasePlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;
	
	ABaseCharacter* BCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BCharacter)
	{
		BCharacter->SetTeamColor(Team);
	}
}

void ABasePlayerState::OnRep_Team()
{
	ABaseCharacter* BCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BCharacter)
	{
		BCharacter->SetTeamColor(Team);
	}
}


