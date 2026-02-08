// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/LagCompensationComponent.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.Property != nullptr
		                     ? PropertyChangedEvent.Property->GetFName()
		                     : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ABaseCharacter* OwnerCharacter = Cast<ABaseCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABasePlayerController* OwnerController = Cast<ABasePlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				const float DamageToCause = Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : Damage;
				
				UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
				return;
			}
			ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() &&
				HitCharacter)
			{
				OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->SingleTripTime
				);
			}
		}
	}
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	/*
	FPredictProjectilePathParams PredictParams;
	PredictParams.bTraceWithChannel =true;
	PredictParams.bTraceWithCollision = true;
	PredictParams.DrawDebugTime = 5.f;
	PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	PredictParams.LaunchVelocity = GetActorForwardVector() * 3500.f;
	PredictParams.MaxSimTime = 4.f;
	PredictParams.ProjectileRadius = 5.f;
	PredictParams.SimFrequency = 30.f;
	PredictParams.StartLocation = GetActorLocation();
	PredictParams.TraceChannel = ECC_Visibility;
	PredictParams.ActorsToIgnore.Add(this);
	
	FPredictProjectilePathResult PredictResult;
	
	UGameplayStatics::PredictProjectilePath(this,PredictParams,PredictResult);
	*/
}
