

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABaseCharacter;

/**
 * @brief 角色增益组件 (BuffComponent)
 * 
 * 该组件负责管理角色的各种增益效果，包括：
 * - 治疗效果：随时间恢复血量
 * - 护盾补充：随时间恢复护盾值
 * - 速度增益：临时提升移动速度（包括行走和蹲伏速度）
 * - 跳跃增益：临时提升跳跃高度
 * 
 * 所有增益效果都支持多人游戏的网络同步
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数，设置组件的Tick属性 */
	UBuffComponent();

	/** 
	 * 每帧更新组件
	 * @param DeltaTime 距离上一帧的时间间隔
	 * @param TickType  ticking类型
	 * @param ThisTickFunction 当前tick函数
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	/** 声明BaseCharacter为友元类，允许其访问私有成员 */
	friend class ABaseCharacter;

	/**
	 * @brief 启动治疗效果
	 * @param HealAmount 总共要恢复的血量
	 * @param HealingTime 治疗持续的时间（秒）
	 */
	void Heal(float HealAmount, float HealingTime);

	/**
	 * @brief 启动护盾补充效果
	 * @param ShieldAmount 总共要恢复的护盾值
	 * @param ReplenishTime 护盾补充持续的时间（秒）
	 */
	void ReplenishShield(float ShieldAmount, float ReplenishTime);

	/**
	 * @brief 启动速度增益效果
	 * @param BuffBaseSpeed 增益后的行走速度
	 * @param BuffCrouchSpeed 增益后的蹲伏速度
	 * @param BuffTime 增益效果的持续时间（秒）
	 */
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);

	/**
	 * @brief 启动跳跃增益效果
	 * @param BuffJumpVelocity 增益后的跳跃速度（Z轴初速度）
	 * @param BuffTime 增益效果的持续时间（秒）
	 */
	void BuffJump(float BuffJumpVelocity, float BuffTime);

	/**
	 * @brief 设置角色的初始速度值（用于增益结束后恢复）
	 * @param BaseSpeed 基础行走速度
	 * @param CrouchSpeed 基础蹲伏速度
	 */
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);

	/**
	 * @brief 设置角色的初始跳跃速度（用于增益结束后恢复）
	 * @param Velocity 基础跳跃Z轴速度
	 */
	void SetInitialJumpVelocity(float Velocity);

protected:
	/** 组件开始播放时调用 */
	virtual void BeginPlay() override;

	/**
	 * @brief 每帧执行治疗递增
	 * @param DeltaTime 距离上一帧的时间间隔
	 */
	void HealRampUp(float DeltaTime);

	/**
	 * @brief 每帧执行护盾递增
	 * @param DeltaTime 距离上一帧的时间间隔
	 */
	void ShieldRampUp(float DeltaTime);

private:
	/** 绑定的角色引用 */
	UPROPERTY()
	ABaseCharacter* Character;

	//============================
	// 治疗相关属性
	//============================
	/** 是否正在治疗中 */
	bool bHealing = false;
	/** 每秒恢复的血量 */
	float HealingRate = 0.f;
	/** 剩余需要恢复的血量总量 */
	float AmountToHeal = 0.f;

	//============================
	// 护盾相关属性
	//============================
	/** 是否正在补充护盾 */
	bool bReplenishShield = false;
	/** 每秒恢复的护盾值 */
	float ShieldReplenishRate = 0.f;
	/** 剩余需要恢复的护盾总量 */
	float ShieldReplenishAmount = 0.f;

	//============================
	// 速度增益相关属性
	//============================
	/** 速度增益定时器句柄 */
	FTimerHandle SpeedBuffTimer;
	/** 重置速度到初始值 */
	void ResetSpeeds();
	/** 初始行走速度（用于恢复） */
	float InitialBaseSpeed;
	/** 初始蹲伏速度（用于恢复） */
	float InitialCrouchSpeed;

	/**
	 * @brief 多播速度增益效果（网络同步）
	 * @param BaseSpeed 行走速度
	 * @param CrouchSpeed 蹲伏速度
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	//============================
	// 跳跃增益相关属性
	//============================
	/** 跳跃增益定时器句柄 */
	FTimerHandle JumpBuffTimer;
	/** 重置跳跃速度到初始值 */
	void ResetJump();
	/** 初始跳跃速度（用于恢复） */
	float InitialJumpVelocity;

	/**
	 * @brief 多播跳跃增益效果（网络同步）
	 * @param JumpVelocity 跳跃Z轴速度
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
