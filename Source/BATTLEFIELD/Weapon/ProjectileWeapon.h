
#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */

class AProjectile;
UCLASS()
class BATTLEFIELD_API AProjectileWeapon : public AWeaponBase
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;
private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;
};
