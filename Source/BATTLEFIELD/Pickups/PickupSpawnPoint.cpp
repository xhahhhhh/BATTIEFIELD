/**
 * @file PickupSpawnPoint.cpp
 * @brief 拾取物生成点实现
 * 
 * 实现拾取物的自动生成和重生逻辑：
 * - 游戏开始时生成首个拾取物
 * - 拾取物被拾取后自动计时重生
 * - 支持从多种类型中随机选择
 */

#include "PickupSpawnPoint.h"

#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	// 启用Tick更新
	PrimaryActorTick.bCanEverTick = true;
	
	// 启用网络复制
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
	// 游戏开始时启动首次生成定时器
	StartSpawnPickupTimer((AActor*)nullptr);
}

void APickupSpawnPoint::SpawnPickup()
{
	// 检查是否有可生成的拾取物类型
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		// 从列表中随机选择一种拾取物
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		
		// 在当前位置生成拾取物
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClasses[Selection], GetActorTransform());

		// 仅在服务器上绑定销毁事件
		if (HasAuthority())
		{
			// 拾取物被销毁时自动启动重生定时器
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	// 仅在服务器上执行生成
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	// 在配置的时间范围内随机选择重生时间
	const float SpawnTime = FMath::RandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	
	// 设置生成定时器
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 当前无每帧逻辑，保留用于未来扩展
}
