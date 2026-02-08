
#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEFIELD_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileBullet();
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
};
