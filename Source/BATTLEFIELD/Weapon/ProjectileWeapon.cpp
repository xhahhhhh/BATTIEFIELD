// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// if (!HasAuthority())return;

	if (ProjectileClass == nullptr) return;
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = OwnerPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (OwnerPawn->HasAuthority())//服务器
			{
				if (OwnerPawn->IsLocallyControlled()) //客户端在服务器上为本地监听，生成复制的子弹，无倒带
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),
					                                                   TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else //客户端在服务器上为模拟代理，生成非复制子弹，开启倒带
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
					                                                   SocketTransform.GetLocation(), TargetRotation,
					                                                   SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else //本地客户端
			{
				if (OwnerPawn->IsLocallyControlled()) //自主代理，生成非复制的子弹，启用倒带
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
					                                                   SocketTransform.GetLocation(), TargetRotation,
					                                                   SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile
						->InitialSpeed;
				}
				else //模拟代理客户端，不使用倒带
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
					                                                   SocketTransform.GetLocation(), TargetRotation,
					                                                   SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else //bUseServerSideRewind为false,不使用倒带
		{
			if (OwnerPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(),
				                                                   TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
}
