// Fill out your copyright notice in the page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "BATTLEFIELD/Weapon/WeaponTypes.h"
#include "AmmoPickup.generated.h"

/**
 * @brief 弹药拾取物 (AmmoPickup)
 * 
 * 为特定武器类型补充弹药的拾取物
 * 
 * 拾取后通过CombatComponent为目标武器增加弹药
 */
UCLASS()
class BATTLEFIELD_API AAmmoPickup : public APickup
{
	GENERATED_BODY()

protected:
	/**
	 * @brief 球形碰撞重叠事件
	 * 
	 * 检测玩家重叠，为目标武器补充弹药后销毁
	 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								 const FHitResult& SweepResult) override;

private:
	/**
	 * @brief 弹药补充数量
	 * 
	 * 拾取后为目标武器增加的弹药数量
	 */
	UPROPERTY(EditAnywhere, Category = "Ammo Pickup")
	int32 AmmoAmount = 30;

	/**
	 * @brief 目标武器类型
	 * 
	 * 此弹药包对应的武器类型
	 */
	UPROPERTY(EditAnywhere, Category = "Ammo Pickup")
	EWeaponType WeaponType;
};
