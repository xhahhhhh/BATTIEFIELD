
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/PlayerHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../CharacterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ABaseCharacter;
class AWeaponBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BATTLEFIELD_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	friend class ABaseCharacter;

	void EquipWeapon(AWeaponBase* WeaponToEquip);
	void SwapWeapons();
	void Reload();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
	UFUNCTION(BlueprintCallable)
	void FinishSwap();
	
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	
	void ShootPressed(bool bPressed);
	
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	
	void JumpToShotgunEnd();
	
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();
	
	UFUNCTION(Server,Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);
	
	void PickupAmmo(EWeaponType WeaponType,int32 AmmoAmount);
	bool bLocallyReloading = false;
protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool bIsAiming);
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	void Shoot();
	void ShootProjectileWeapon();
	void ShootHitScanWeapon();
	void ShootShotgun();
	void LocalShoot(const FVector_NetQuantize& TraceHitTarget);
	void LocalShotgunShoot(const TArray<FVector_NetQuantize>& TraceHitTargets);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget,float FireDelay);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets,float FireDelay);
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrossHairs(float DeltaTime);
	
	UFUNCTION(Server,Reliable)
	void ServerReload();

	void HandleReload();
	int32 AmountToReload();
	
	void ThrowGrenade();
	
	UFUNCTION(Server,Reliable)
	void Server_ThrowGrenade();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;
	
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void AttachFlagToRightHand(AWeaponBase* Flag);
	void UpdateCarriedAmmo();
	void PlayEquipSound(AWeaponBase* WeaponToEquip);
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);
	void EquipPrimaryWeapon(AWeaponBase* WeaponToEquip);
	void EquipSecondaryWeapon(AWeaponBase* WeaponToEquip);
private:
	UPROPERTY()
	ABaseCharacter* Character;
	UPROPERTY()
	class ABasePlayerController* Controller;
	UPROPERTY()
	class APlayerHUD* HUD;
	
	FVector2D ViewportSize;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeaponBase* EquippedWeapon;
	
	UPROPERTY(ReplicatedUsing =OnRep_SecondaryWeapon)
	AWeaponBase* SecondaryWeapon;

	UPROPERTY(Replicated,ReplicatedUsing=OnRep_Aiming)
	bool bAiming = false;
	
	bool bAimButtonPressed = false;

	UFUNCTION()
	void OnRep_Aiming();
	
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bShootPressed;


	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector End;
	FHitResult HitResult;
	FVector HitTarget;
	FHUDPackage HUDPackage;

	//��׼FOV

	//Ĭ��FOV
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	//ȫ�Զ�
	FTimerHandle FireTimer;

	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();
	
	bool CanFire();
	
	UPROPERTY(ReplicatedUsing= OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	
	UFUNCTION()
	void OnRep_CarriedAmmo();
	
	TMap<EWeaponType,int32> CarriedAmmoMap;
	
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;
	
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;
	
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingShotGunAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;
	
	void InitializeCarriedAmmo();
	
	UPROPERTY(ReplicatedUsing=OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;
	
	UFUNCTION()
	void OnRep_CombatState();
	
	void UpdateAmmoValues();
	
	void UpdateShotgunAmmoValues();
	
	UPROPERTY(ReplicatedUsing=OnRep_Grenades)
	int32 Grenades = 1;
	
	UFUNCTION()
	void OnRep_Grenades();
	
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;
	
	void UpdateHUDGrenades();
	
	UPROPERTY(ReplicatedUsing=OnRep_HoldingTheFlag)
	bool bHoldTheFlag = false;
	
	UFUNCTION()
	void OnRep_HoldingTheFlag();
	
	UPROPERTY()
	AWeaponBase* TheFlag;
public:
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	bool ShouldSwapWeapons();
};
