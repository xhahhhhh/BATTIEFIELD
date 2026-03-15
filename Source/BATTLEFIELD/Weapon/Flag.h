#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "Flag.generated.h"

/**
 * @brief 旗帜类 (Flag)
 * 
 * 夺旗模式中的旗帜物品，继承自武器系统
 * 
 * 特性：
 * - 特殊的外观（静态网格）
 * - 记录初始位置（用于重置）
 * - 持有者蹲下时掉落
 * - 回合结束时重置到初始位置
 */
UCLASS()
class BATTLEFIELD_API AFlag : public AWeaponBase
{
	GENERATED_BODY()

public:
	/** 构造函数，创建旗帜网格 */
	AFlag();

	/**
	 * @brief 掉落旗帜
	 * 
	 * 与基类不同，旗帜使用 FlagMesh 进行分离
	 */
	virtual void Dropped() override;

	/**
	 * @brief 重置旗帜到初始位置
	 * 
	 * 调用时机：
	 * - 持有者被淘汰
	 * - 回合结束
	 * - 旗帜被成功送达
	 */
	void ResetFlag();

protected:
	/** 组件开始播放时调用，记录初始变换 */
	virtual void BeginPlay() override;

	/** 
	 * @brief 装备时调用
	 * 
	 * 设置网格属性：
	 * - 禁用物理模拟
	 * - 禁用重力
	 * - 启用仅查询碰撞
	 */
	virtual void OnEquipped() override;

	/**
	 * @brief 掉落时调用
	 * 
	 * 设置网格属性：
	 * - 启用物理模拟和重力
	 * - 启用完整碰撞
	 * - 启用轮廓高亮
	 */
	virtual void OnDropped() override;

private:
	/** 旗帜网格组件 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;

	/** 初始变换（用于重置） */
	FTransform InitialTransform;

public:
	/** 获取初始变换 */
	FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }
};
