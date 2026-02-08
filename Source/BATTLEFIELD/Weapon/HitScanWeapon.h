
#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "HitScanWeapon.generated.h"

class UParticleSystem;

UCLASS()
class BATTLEFIELD_API AHitScanWeapon : public AWeaponBase
{
	GENERATED_BODY()
	
public:
	virtual void Fire(const FVector& HitTarget)override;
	
	// UPROPERTY(EditAnywhere)
	// float Damage = 20.f;
	
protected:
	void WeaponTraceHit(const FVector& TraceStart,const FVector& HitTarget, FHitResult& OutHit);
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;
	
	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
	
};
