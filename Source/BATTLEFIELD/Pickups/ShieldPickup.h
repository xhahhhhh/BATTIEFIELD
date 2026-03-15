
#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

/**
 * @brief 护盾拾取物 (ShieldPickup)
 * 
 * 为玩家提供持续护盾恢复效果的拾取物
 * 
 * 拾取后通过BuffComponent在指定时间内持续恢复护盾值
 */
UCLASS()
class BATTLEFIELD_API AShieldPickup : public APickup
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	AShieldPickup();

protected:
	/**
	 * @brief 球形碰撞重叠事件
	 * 
	 * 检测玩家重叠，启动护盾恢复效果后销毁
	 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
							 const FHitResult& SweepResult) override;

private:
	/**
	 * @brief 护盾恢复总量
	 * 
	 * 拾取后在恢复时间内总共恢复的护盾值
	 */
	UPROPERTY(EditAnywhere, Category = "Shield Pickup")
	float ShieldReplenishAmount = 100.f;

	/**
	 * @brief 护盾恢复持续时间（秒）
	 * 
	 * 护盾恢复效果的持续时间
	 */
	UPROPERTY(EditAnywhere, Category = "Shield Pickup")
	float ShieldReplenishTime = 3.f;
};
