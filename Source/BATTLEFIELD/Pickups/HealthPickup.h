// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "HealthPickup.generated.h"

/**
 * @brief 治疗拾取物 (HealthPickup)
 * 
 * 为玩家提供持续治疗效果的拾取物
 * 
 * 拾取后通过BuffComponent在指定时间内持续恢复血量
 */
UCLASS()
class BATTLEFIELD_API AHealthPickup : public APickup
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	AHealthPickup();

protected:
	/**
	 * @brief 球形碰撞重叠事件
	 * 
	 * 检测玩家重叠，启动治疗效果后销毁
	 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								 const FHitResult& SweepResult) override;

private:
	/**
	 * @brief 治疗总量
	 * 
	 * 拾取后在治疗时间内总共恢复的血量
	 */
	UPROPERTY(EditAnywhere, Category = "Health Pickup")
	float HealAmount = 100.f;

	/**
	 * @brief 治疗持续时间（秒）
	 * 
	 * 血量恢复效果的持续时间
	 */
	UPROPERTY(EditAnywhere, Category = "Health Pickup")
	float HealingTime = 3.f;
};
