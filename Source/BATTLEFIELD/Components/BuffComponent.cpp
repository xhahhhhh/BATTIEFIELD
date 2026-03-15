/**
 * @file BuffComponent.cpp
 * @brief 角色增益组件实现
 * 
 * 实现角色的各种增益效果：治疗、护盾、速度提升、跳跃提升
 * 所有效果都支持随时间渐变，并且正确同步到网络客户端
 */

#include "BuffComponent.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

//============================
// 构造函数和生命周期函数
//============================

UBuffComponent::UBuffComponent()
{
	// 启用每帧Tick更新，用于处理治疗和护盾的渐变效果
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	// 获取绑定的角色引用
	Character = Cast<ABaseCharacter>(GetOwner());
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 每帧处理治疗效果的递增
	HealRampUp(DeltaTime);
	// 每帧处理护盾效果的递增
	ShieldRampUp(DeltaTime);
}

//============================
// 治疗功能
//============================

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
	// 标记开始治疗
	bHealing = true;
	
	// 计算每秒恢复的血量 = 总恢复量 / 持续时间
	HealingRate = HealAmount / HealingTime;
	// 累加需要恢复的血量（支持多次治疗叠加）
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
{
	// 检查治疗状态：未在治療、角色无效、或角色已被淘汰时跳过
	if (!bHealing || Character == nullptr || Character->IsElimmed()) return;

	// 计算本帧恢复的血量
	const float HealingThisFrame = DeltaTime * HealingRate;
	
	// 更新角色血量并限制在有效范围内 [0, MaxHealth]
	Character->SetHealth(FMath::Clamp(
		Character->GetHealth() + HealingThisFrame, 
		0, 
		Character->GetMaxHealth()
	));
	
	// 更新HUD显示
	Character->UpdateHUDHealth();
	
	// 减少剩余待恢复血量
	AmountToHeal -= HealingThisFrame;

	// 治疗结束条件：待恢复量为0 或 血量已满
	if (AmountToHeal <= 0.0f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.0f;
	}
}

//============================
// 护盾功能
//============================

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
	// 标记开始补充护盾
	bReplenishShield = true;
	
	// 计算每秒恢复的护盾值 = 总恢复量 / 持续时间
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	// 累加需要恢复的护盾值（支持多次补充叠加）
	ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
	// 检查护盾状态：未在补充、角色无效、或角色已被淘汰时跳过
	if (!bReplenishShield || Character == nullptr || Character->IsElimmed()) return;

	// 计算本帧恢复的护盾值
	const float ReplenishThisFrame = DeltaTime * ShieldReplenishRate;
	
	// 更新角色护盾值并限制在有效范围内 [0, MaxShield]
	Character->SetShield(FMath::Clamp(
		Character->GetShield() + ReplenishThisFrame, 
		0, 
		Character->GetMaxShield()
	));
	
	// 更新HUD显示
	Character->UpdateHUDShield();
	
	// 减少剩余待恢复护盾值
	ShieldReplenishAmount -= ReplenishThisFrame;

	// 补充结束条件：待恢复量为0 或 护盾已满
	if (ShieldReplenishAmount <= 0.0f || Character->GetShield() >= Character->GetMaxShield())
	{
		bReplenishShield = false;
		ShieldReplenishAmount = 0.0f;
	}
}

//============================
// 速度增益功能
//============================

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	// 保存初始速度值，用于增益结束后恢复
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime)
{
	// 安全检查：角色必须有效
	if (Character == nullptr) return;
	
	// 设置定时器，在BuffTime后自动重置速度
	Character->GetWorldTimerManager().SetTimer(
		SpeedBuffTimer,           // 定时器句柄
		this,                     // 回调对象
		&UBuffComponent::ResetSpeeds, // 回调函数
		BuffTime                  // 延迟时间（秒）
	);
	
	// 直接在服务器上修改角色移动速度
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	
	// 多播同步到所有客户端
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::ResetSpeeds()
{
	// 安全检查：角色和移动组件必须有效
	if (Character == nullptr || Character->GetCharacterMovement() == nullptr) return;
	
	// 恢复初始速度
	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	
	// 多播同步到所有客户端
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	// 在客户端上应用速度修改
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	}
}

//============================
// 跳跃增益功能
//============================

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	// 保存初始跳跃速度，用于增益结束后恢复
	InitialJumpVelocity = Velocity;
}

void UBuffComponent::BuffJump(float BuffJumpVelocity, float BuffTime)
{
	// 安全检查：角色必须有效
	if (Character == nullptr) return;
	
	// 设置定时器，在BuffTime后自动重置跳跃速度
	Character->GetWorldTimerManager().SetTimer(
		JumpBuffTimer,            // 定时器句柄
		this,                     // 回调对象
		&UBuffComponent::ResetJump, // 回调函数
		BuffTime                  // 延迟时间（秒）
	);
	
	// 直接在服务器上修改角色跳跃速度
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	
	// 多播同步到所有客户端
	MulticastJumpBuff(BuffJumpVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	// 在客户端上应用跳跃速度修改
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
	}
}

void UBuffComponent::ResetJump()
{
	// 恢复初始跳跃速度
	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	}
	
	// 多播同步到所有客户端
	MulticastJumpBuff(InitialJumpVelocity);
}

