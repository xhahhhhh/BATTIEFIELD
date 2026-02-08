// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameState.h"

#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "BATTLEFIELD/PlayerState/BasePlayerState.h"
#include "Net/UnrealNetwork.h"

void ABaseGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseGameState,TopScoringPlayers);
	DOREPLIFETIME(ABaseGameState,RedTeamScore);
	DOREPLIFETIME(ABaseGameState,BlueTeamScore);
}

void ABaseGameState::UpdateTopScore(ABasePlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() ==TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() >TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABaseGameState::RedTeamScores()
{
	++RedTeamScore;
	
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABaseGameState::BlueTeamScores()
{
	++BlueTeamScore;
	
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABaseGameState::OnRep_RedTeamScore()
{
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABaseGameState::OnRep_BlueTeamScore()
{
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
