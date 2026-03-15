/**
 * @file ProjectileWeapon.cpp
 * @brief 投射物武器实现
 * 
 * 实现投射物武器的射击逻辑：
 * - 根据网络情况选择投射物类型
 * - 支持服务器倒带补偿
 */

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (ProjectileClass == nullptr) return;
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlashSocket");
	UWorld* World = GetWorld();
	
	if (MuzzleFlashSocket && World)
	{
		// 获取枪口变换
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		// 设置生成参数
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = OwnerPawn;

		AProjectile* SpawnedProjectile = nullptr;
		
		// 根据服务器倒带设置选择投射物类型
		if (bUseServerSideRewind)
		{
			if (OwnerPawn->HasAuthority()) // 服务器
			{
				if (OwnerPawn->IsLocallyControlled()) 
				{
					// 服务器本地控制（监听服务器主机）
					// 生成复制投射物，不开启倒带
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, 
						SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
				}
				else 
				{
					// 服务器上的模拟代理客户端
					// 生成非复制投射物，开启倒带
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
						SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
				}
			}
			else // 本地客户端
			{
				if (OwnerPawn->IsLocallyControlled()) 
				{
					// 自主代理（本地玩家）
					// 生成非复制投射物，启用倒带用于预测
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
						SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * 
						SpawnedProjectile->InitialSpeed;
				}
				else 
				{
					// 模拟代理（其他玩家）
					// 生成非复制投射物，不使用倒带
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,
						SocketTransform.GetLocation(), TargetRotation, SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // 不使用服务器倒带
		{
			if (OwnerPawn->HasAuthority())
			{
				// 服务器生成复制投射物
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass, 
					SocketTransform.GetLocation(), TargetRotation, SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
}
