// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayerController.generated.h"

class ABaseGameState;
class ABasePlayerState;
class UInputAction;
class UInputMappingContext;
class APlayerState;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool,bPingTooHigh);

UCLASS()
class BATTLEFIELD_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);
	
	//同步客户端与服务器的游戏时间
	virtual float GetServerTime();
	//服务器同步
	virtual void ReceivedPlayer() override;

	void OnMatchStateSet(FName State,bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	void HandleCooldown();
	
	float SingleTripTime = 0.f;
	
	FHighPingDelegate HighPingDelegate;
	
	void BroadcastElim(APlayerState* Attacker,APlayerState* Victim);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void SetHUDTime();

	void PollInit();

	//计算客户端与服务器的时间延时

	//请求当前服务器时间，在请求发送时记录客户端的时间
	UFUNCTION(Server, reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//返回服务器端时间并响应给ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);
	
	//保存差值
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category=Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Server, reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
	
	void ShowReturnToMainMenu();
	
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker,APlayerState* Victim);
	
	UPROPERTY(ReplicatedUsing=OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	
	UFUNCTION()
	void OnRep_ShowTeamScores();
	
	FString GetInfoText(const TArray<ABasePlayerState*>& Players);
	FString GetTeamsInfoText(ABaseGameState* BaseGameState);
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* ControllerMappingContext;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Quit;

private:
	UPROPERTY()
	class APlayerHUD* PlayerHUD;

	UPROPERTY()
	class APlayerGameMode* PlayerGameMode;
	
	UPROPERTY(EditAnywhere,Category=HUd)
	TSubclassOf<class UUserWidget> PauseMenuWidget;
	
	UPROPERTY()
	class UPauseMenu* PauseMenu;
	
	bool bPauseMenuOpen = false;

	float LevelStartTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;

	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;


	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;
	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;
	
	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;
	
	UFUNCTION(server, reliable)
	void ServerReportPingStatus(bool bHighPing);
	
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;
};
