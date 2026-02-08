#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "WeaponBase.generated.h"

class USphereComponent;
class UWidgetComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class EWeaponState :uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped State"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped State"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType :uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScanWeapon"),
	EFT_Projectile UMETA(DisplayName = "ProjectileWeapon"),
	EFT_Shotgun UMETA(DisplayName = "ShotgunWeapon"),
	
	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BATTLEFIELD_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	void SetHUDAmmo();
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	virtual void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	FVector TraceEndWithScatter(const  FVector& HitTarget);

protected:
	UPROPERTY()
	class ABaseCharacter* OwnerCharacter;

	UPROPERTY()
	class ABasePlayerController* OwnerController;
	
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                             const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	//散布
	UPROPERTY(EditAnywhere,Category="WeaponScatter")
	float DistanceToSphere = 800.f;
	
	UPROPERTY(EditAnywhere,Category="WeaponScatter")
	float SphereRadius = 75.f;
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
	
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;
	
	UPROPERTY(Replicated,EditAnywhere)
	bool bUseServerSideRewind = false;
	
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickUpWidget;

	UPROPERTY(EditAnywhere, Category = "WeaponProperties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);
	// UFUNCTION()
	// void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	//计数服务器未处理的请求（弹药）
	//弹药减少 + 弹药增加 - 
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:
	//׼�Ĳ���
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsButtom;

	//��׼����
	UPROPERTY(EditAnywhere)
	float ZoomenFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	//ȫ�Զ�
	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true;
	
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;
	
	//自定义后处理深度
	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;
	
	UPROPERTY(EditAnywhere)
	EFireType FireType;
	
	UPROPERTY(EditAnywhere)
	ETeam Team;
	
	UPROPERTY(EditAnywhere,Category="WeaponScatter")
	bool bUseScatter = false;
public:
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UWidgetComponent* GetPickUpWidget() const { return PickUpWidget; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomenFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	bool IsEmpty();
	bool IsFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
