
#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "JumpPickup.generated.h"

/**
 * @brief 跳跃增益拾取物 (JumpPickup)
 * 
 * 临时提升玩家跳跃高度的拾取物
 * 
 * 拾取后通过BuffComponent在一定时间内提升跳跃速度
 */
UCLASS()
class BATTLEFIELD_API AJumpPickup : public APickup
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	AJumpPickup();

protected:
	/**
	 * @brief 球形碰撞重叠事件
	 * 
	 * 检测玩家重叠，启动跳跃增益效果后销毁
	 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								 const FHitResult& SweepResult) override;

private:
	/**
	 * @brief 增益后的跳跃Z轴速度
	 * 
	 * 速度增益期间的跳跃初速度（决定跳跃高度）
	 */
	UPROPERTY(EditAnywhere, Category = "Jump Pickup")
	float JumpZVelocityBuff = 4000.f;

	/**
	 * @brief 跳跃增益持续时间（秒）
	 * 
	 * 跳跃提升效果的持续时间，结束后自动恢复
	 */
	UPROPERTY(EditAnywhere, Category = "Jump Pickup")
	float JumpBuffTime = 30.f;
};
