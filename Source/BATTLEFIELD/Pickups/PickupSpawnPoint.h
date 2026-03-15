
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

/**
 * @brief 拾取物生成点 (PickupSpawnPoint)
 * 
 * 管理拾取物的生成和重生：
 * - 从配置的拾取物类型中随机选择生成
 * - 拾取物被拾取后自动计时重生
 * - 支持随机重生时间范围配置
 * 
 * 在地图中放置此Actor并配置PickupClasses即可创建拾取点
 */
UCLASS()
class BATTLEFIELD_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	APickupSpawnPoint();

	/**
	 * @brief 每帧更新
	 * @param DeltaTime 距离上一帧的时间间隔
	 */
	virtual void Tick(float DeltaTime) override;

protected:
	/** 游戏开始时初始化，开始首次生成 */
	virtual void BeginPlay() override;

	/**
	 * @brief 可生成的拾取物类型列表
	 * 
	 * 从此数组中随机选择一种拾取物生成
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Spawn")
	TArray<TSubclassOf<APickup>> PickupClasses;

	/**
	 * @brief 当前生成的拾取物实例
	 * 
	 * 用于监听销毁事件，触发重生
	 */
	UPROPERTY()
	APickup* SpawnedPickup;

	/**
	 * @brief 生成拾取物
	 * 
	 * 从PickupClasses中随机选择一种并在当前位置生成
	 */
	void SpawnPickup();

	/**
	 * @brief 生成定时器完成回调
	 * 
	 * 在服务器上执行实际生成操作
	 */
	void SpawnPickupTimerFinished();

	/**
	 * @brief 启动拾取物生成定时器
	 * @param DestroyedActor 被销毁的Actor（绑定到拾取物的OnDestroyed事件）
	 * 
	 * 设置随机延迟后生成新拾取物
	 */
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:
	/**
	 * @brief 生成定时器句柄
	 */
	FTimerHandle SpawnPickupTimer;

	/**
	 * @brief 最短重生时间（秒）
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Spawn")
	float SpawnPickupTimeMin = 5.f;

	/**
	 * @brief 最长重生时间（秒）
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Spawn")
	float SpawnPickupTimeMax = 15.f;
};
