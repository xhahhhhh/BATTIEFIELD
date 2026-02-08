// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BaseGameState.generated.h"

/**
 * 
 */
class ABasePlayerState;

UCLASS()
class BATTLEFIELD_API ABaseGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void UpdateTopScore(ABasePlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<ABasePlayerState*> TopScoringPlayers;
	
	//队伍
	void RedTeamScores();
	void BlueTeamScores();
	
	TArray<ABasePlayerState*> RedTeam;
	TArray<ABasePlayerState*> BlueTeam;
	
	UPROPERTY(ReplicatedUsing=OnRep_RedTeamScore)
	float RedTeamScore =0.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_BlueTeamScore)
	float BlueTeamScore =0.f;
	
	UFUNCTION()
	void OnRep_RedTeamScore();
	
	UFUNCTION()
	void OnRep_BlueTeamScore();
private:
	float TopScore = 0.f;
};
