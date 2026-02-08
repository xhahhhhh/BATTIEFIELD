#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../CharacterTypes/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "../CharacterTypes/CombatState.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "BaseCharacter.generated.h"

class APlayerGameMode;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UInputMappingContext;
class UWidgetComponent;
class AWeaponBase;
class UCombatComponent;
class UAnimMontage;
class AController;
class UBuffComponent;
class UBoxComponent;
UCLASS()
class BATTLEFIELD_API ABaseCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	//UFUNCTION(NetMulticast, Unreliable)
	//void MulticastHit();
	virtual void OnRep_ReplicatedMovement() override;
	void Elim(bool bPlayerLeftGame);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	
	void SpawnDefaultWeapon();

	UPROPERTY()
	TMap<FName,UBoxComponent*> HitCollisionBoxes;
	
	bool bFinishedSwapping = false;
	
	UFUNCTION(Server,Reliable)
	void ServerLeaveGame();
	
	FOnLeftGame OnLeftGame;
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastGainedTheLead();
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastLostTheLead();
	
	void SetTeamColor(ETeam Team);
	
protected:
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* MappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Look;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Jump;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Equip;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Crouch;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Aim;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Shoot;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Reload;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Throw;

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void DoJump();
	void EquipButtonPressed();
	void DoCrouch();
	void ReloadPressed();
	void Aim();
	void AimEnd();
	virtual void Jump() override;
	void ShootPressed();
	void ShootReleased();
	void ThrowPressed();

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	void PlayHitReactMontage();

	void DropOrDestroyWeapon(AWeaponBase* Weapon);

	void DropOrDestroyWeapons();
	
	void SetSpawnPoint();
	void OnPlayerStateInitialized();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	                   AController* InstigatorController, AActor* DamageCauser);
	//Init HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);

	//用于服务器倒带的命中框
	UPROPERTY(EditAnywhere)
	UBoxComponent* head;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_01;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_04;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_05;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverHeadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeaponBase* OverlappingWeapon;

	//Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBuffComponent* Buff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class ULagCompensationComponent* LagCompensation;
	
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeaponBase* Lastweapon);

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	
	void TurnInPlace(float DeltaTime);

	//Montages
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = .5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed();

	//生命值
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxHealth = 100.f;

	UPROPERTY(Replicated = OnRep_Health, VisibleAnywhere, Category = "PlayerStats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	//护盾值
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxShield = 100.f;

	UPROPERTY(Replicated = OnRep_Shield, EditAnywhere, Category = "PlayerStats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABasePlayerController* BasePlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();
	
	bool bLeftGame = false;
	
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	//MaterialInstance
	UPROPERTY(VisibleAnywhere, Category=Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(VisibleAnywhere, Category=Elim)
	UMaterialInstance* DissolveMaterialInstance;
	
	//团队标识
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* RedDissolveMatInst;
	
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* RedMaterial;
	
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* BlueDissolveMatInst;
	
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* BlueMaterial;
	
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* OriginalMaterial;
	//死亡特效
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimEffectComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimSound;

	UPROPERTY()
	class ABasePlayerState* BasePlayerState;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;
	
	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;
	
	//默认持有武器
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY()
	APlayerGameMode* PlayerGameMode;
public:
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeaponBase* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	bool IsLocallyReloading();
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE bool IsHoldingTheFlag() const;
	ETeam GetTeam();
	void SetHoldingTheFlag(bool bHolding);
};


