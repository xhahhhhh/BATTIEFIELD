
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "../CharacterTypes/TurningInPlace.h"
#include "BaseAnimInstance.generated.h"

class ABaseCharacter;

/**
 * @class UBaseAnimInstance
 * @brief 基础角色动画实例类
 * 
 * 该类负责管理角色的动画状态，包括：
 * - 基础移动状态（行走速度、跳跃、加速、蹲伏）
 * - 武器相关状态（装备武器、瞄准、IK位置）
 * - 身体倾斜和转身逻辑
 * - 瞄准偏移（Aim Offset）计算
 * - 左手IK（握枪位置）和右手旋转同步
 * - FABRIK IK控制
 */
UCLASS()
class BATTLEFIELD_API UBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	/** 
	 * @brief 动画初始化时调用
	 * 获取并缓存拥有此动画实例的角色引用
	 */
	virtual void NativeInitializeAnimation() override;
	
	/**
	 * @brief 每帧更新动画
	 * @param DeltaSeconds 距离上一帧的时间间隔
	 * 更新所有动画相关的状态变量，用于驱动动画蓝图
	 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	/** @brief 拥有此动画实例的角色引用 */
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	ABaseCharacter* OwningCharacter;

	/** @brief 角色水平移动速度（用于控制行走/奔跑动画） */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	/** @brief 角色是否在空中（跳跃/下落状态） */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	/** @brief 角色是否在加速（用于判断起步/停止动画过渡） */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	/** @brief 角色是否装备了武器 */
	UPROPERTY(BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	/** @brief 当前装备的武器实例 */
	UPROPERTY()
	class AWeaponBase* EquippedWeapon;

	/** @brief 角色是否处于蹲伏状态 */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsCrouched;

	/** @brief 角色是否处于瞄准状态 */
	UPROPERTY(BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	/**
	 * @brief 角色移动方向与瞄准方向的Yaw角度偏移
	 * 用于混合空间（BlendSpace）控制角色倾斜/转向动画
	 */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset;

	/**
	 * @brief 角色身体倾斜角度
	 * 根据角色旋转速度计算，范围为[-90, 90]
	 */
	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Lean;

	/** @brief 瞄准偏移Yaw值（水平方向） */
	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	float AO_Yaw;

	/** @brief 瞄准偏移Pitch值（垂直方向） */
	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	float AO_Pitch;

	/**
	 * @brief 左手相对于右手骨骼的变换
	 * 用于IK将左手正确放置到武器握把位置
	 */
	UPROPERTY(BlueprintReadOnly, Category = IK, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	/** @brief 原地转身状态（用于转向动画） */
	UPROPERTY(BlueprintReadOnly, Category = Turning, meta = (AllowPrivateAccess = "true"))
	ETurningInPlace TurningInPlace;

	/**
	 * @brief 右手骨骼的目标旋转
	 * 用于本地控制时同步武器与准心指向
	 */
	UPROPERTY(BlueprintReadOnly, Category = IK, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;

	/** @brief 是否为本地控制的角色（区分本地玩家和远程玩家） */
	UPROPERTY(BlueprintReadOnly, Category = Network, meta = (AllowPrivateAccess = "true"))
	bool bLocallyControlled;

	/** @brief 是否需要旋转根骨骼（用于原地转身时固定下半身） */
	UPROPERTY(BlueprintReadOnly, Category = Turning, meta = (AllowPrivateAccess = "true"))
	bool bRotateRootBone;
	
	/** @brief 角色是否已被淘汰（死亡） */
	UPROPERTY(BlueprintReadOnly, Category = State, meta = (AllowPrivateAccess = "true"))
	bool bElimmed;
	
	/**
	 * @brief 是否使用FABRIK IK
	 * 在换弹、投掷手雷等动作时禁用
	 */
	UPROPERTY(BlueprintReadOnly, Category = IK, meta = (AllowPrivateAccess = "true"))
	bool bUseFABRIK;
	
	/**
	 * @brief 是否使用瞄准偏移
	 * 在换弹、禁用游戏玩法时禁用
	 */
	UPROPERTY(BlueprintReadOnly, Category = AimOffset, meta = (AllowPrivateAccess = "true"))
	bool bUseAimOffsets;
	
	/**
	 * @brief 是否变换右手骨骼
	 * 控制是否同步右手旋转以匹配准心
	 */
	UPROPERTY(BlueprintReadOnly, Category = IK, meta = (AllowPrivateAccess = "true"))
	bool bTransformRightHand;
	
	/** @brief 角色是否持有旗帜（夺旗模式） */
	UPROPERTY(BlueprintReadOnly, Category = GameMode, meta = (AllowPrivateAccess = "true"))
	bool bHoldingTheFlag;

	/** @brief 上一帧的角色旋转（用于计算旋转速度） */
	FRotator CharacterRotationLastFrame;
	
	/** @brief 当前帧的角色旋转 */
	FRotator CharacterRotation;
	
	/** @brief 平滑后的旋转差值（用于计算YawOffset） */
	FRotator DeltaRotation;

};
