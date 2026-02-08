#pragma once

#include "CoreMinimal.h"
#include "BATTLEFIELD/Weapon/WeaponBase.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class AWeaponBase;
class ABaseCharacter;

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABaseCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<ABaseCharacter*, uint32> HeadShots;

	UPROPERTY()
	TMap<ABaseCharacter*, uint32> BodyShots;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULagCompensationComponent();
	friend class ABaseCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
	
	//常规倒带
	FServerSideRewindResult ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                                         const FVector_NetQuantize& HitLocation, float HitTime);

	//子弹倒带
	FServerSideRewindResult ProjectileServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
											 const FVector_NetQuantize100& InitialVelocity, float HitTime);

	
	//为霰弹枪定制的倒带
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABaseCharacter*>& HitCharacters,
														   const FVector_NetQuantize& TraceStart,
														   const TArray<FVector_NetQuantize>& HitLocations,
														   float HitTime);

	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter,
									   const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);

	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package,ABaseCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
													 const FVector_NetQuantize& TraceStart,
													 const TArray<FVector_NetQuantize>& HitLocations);

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                        const FVector_NetQuantize& HitLocation, float HitTime);
	
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
											 const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(const TArray<ABaseCharacter*>& HitCharacters,
	                               const FVector_NetQuantize& TraceStart,
	                               const TArray<FVector_NetQuantize>& HitLocations,
	                               float HitTime);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame,
	                                  float HitTime);
	
	void CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	void SaveFramePackage();
	FFramePackage GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime);

	
private:
	UPROPERTY()
	ABaseCharacter* Character;

	UPROPERTY()
	class ABasePlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
