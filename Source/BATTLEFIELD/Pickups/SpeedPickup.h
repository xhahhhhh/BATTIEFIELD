// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "SpeedPickup.generated.h"

/**
 * @brief 速度增益拾取物 (SpeedPickup)
 * 
 * 临时提升玩家移动速度的拾取物
 * 
 * 拾取后通过BuffComponent在一定时间内提升行走和蹲伏速度
 */
UCLASS()
class BATTLEFIELD_API ASpeedPickup : public APickup
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	ASpeedPickup();

protected:
	/**
	 * @brief 球形碰撞重叠事件
	 * 
	 * 检测玩家重叠，启动速度增益效果后销毁
	 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                             const FHitResult& SweepResult) override;

private:
	/**
	 * @brief 增益后的行走速度
	 * 
	 * 速度增益期间的行走移动速度
	 */
	UPROPERTY(EditAnywhere, Category = "Speed Pickup")
	float BaseSpeedBuff = 1600.f;

	/**
	 * @brief 增益后的蹲伏速度
	 * 
	 * 速度增益期间的蹲伏移动速度
	 */
	UPROPERTY(EditAnywhere, Category = "Speed Pickup")
	float CrouchSpeedBuff = 850.f;

	/**
	 * @brief 速度增益持续时间（秒）
	 * 
	 * 速度提升效果的持续时间，结束后自动恢复
	 */
	UPROPERTY(EditAnywhere, Category = "Speed Pickup")
	float SpeedBuffTime = 10.f;
};
