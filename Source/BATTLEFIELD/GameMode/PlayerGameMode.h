// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PlayerGameMode.generated.h"

class ABasePlayerState;

namespace MatchState
{
	extern BATTLEFIELD_API const FName Cooldown; //对局结束冷却等待下一局开始
	
}

class ABasePlayerController;
UCLASS()
class BATTLEFIELD_API APlayerGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	APlayerGameMode();
	virtual void Tick( float DeltaTime );
	virtual void PlayerEliminated(class ABaseCharacter* EliminatedCharacter,ABasePlayerController* VictimController,ABasePlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter,AController* ElimmedController);
	
	void PlayerLeftGame(ABasePlayerState* PlayerLeaving);
	
	virtual float CalculateDamage(AController* Attacker,AController* Victim,float BaseDamage);
	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 180.f;
	
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f ;
	
	float LevelStartingTime = 0.f;
	
	bool bTeamMatch = false;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
private:
	float CountdownTime = 0.f;
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
