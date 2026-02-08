
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UParticleSystem;
class UBoxComponent;
class USoundCue;

UCLASS()
class BATTLEFIELD_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	
	//服务器倒带
	bool bUseServerSideRewind =false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;
	
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	UPROPERTY(EditAnywhere)
	float Damage =20.f;
	
	UPROPERTY(EditAnywhere)
	float HeadShotDamage =40.f;
protected:
	virtual void BeginPlay() override;
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void SpawnTrailSystem();
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;
	
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;
	
	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;
private:
	
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	FTimerHandle DestroyTimer;
	
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
};
