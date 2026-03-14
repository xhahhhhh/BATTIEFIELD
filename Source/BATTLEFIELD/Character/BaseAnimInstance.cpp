#include "BaseAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Weapon/WeaponBase.h"
#include "../CharacterTypes/CombatState.h"
#include "Kismet/KismetMathLibrary.h"

void UBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 初始化时尝试获取拥有此动画实例的角色
	// TryGetPawnOwner() 返回拥有该动画实例的Pawn
	OwningCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
}

void UBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 如果角色引用为空，尝试重新获取
	// 这在动画实例初始化时或角色切换时可能发生
	if (OwningCharacter == nullptr)
	{
		OwningCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
	}
	// 如果仍然为空，说明没有有效的角色拥有此动画实例，直接返回
	if (OwningCharacter == nullptr) return;

	// ========== 【1. 基础移动状态更新】 ==========
	
	// 获取角色水平移动速度（忽略Z轴）
	// 用于控制行走/奔跑/静止动画的混合
	Speed = OwningCharacter->GetVelocity().Size2D();
	
	// 检查角色是否处于空中（跳跃或下落）
	// 用于触发跳跃/下落动画
	bIsInAir = OwningCharacter->GetCharacterMovement()->IsFalling();
	
	// 检查角色是否正在加速（输入了移动指令）
	// 用于区分主动移动和惯性滑行，控制起步/停止动画过渡
	bIsAccelerating = OwningCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size2D() > 0.f ? true : false;

	// ========== 【2. 武器和状态更新】 ==========
	
	// 获取武器相关状态
	bWeaponEquipped = OwningCharacter->IsWeaponEquipped();
	EquippedWeapon = OwningCharacter->GetEquippedWeapon();
	bIsCrouched = OwningCharacter->bIsCrouched;
	bAiming = OwningCharacter->IsAiming();
	TurningInPlace = OwningCharacter->GetTurningInPlace();
	bRotateRootBone = OwningCharacter->ShouldRotateRootBone();
	bElimmed = OwningCharacter->IsElimmed();
	bHoldingTheFlag = OwningCharacter->IsHoldingTheFlag();

	// ========== 【3. YawOffset计算（用于BlendSpace）】 ==========
	
	// 计算角色移动方向与瞄准方向的角度差
	// 这个值用于混合空间（BlendSpace）控制角色朝向动画
	// 例如：向前跑时向右看，身体会有相应的倾斜动画
	FRotator AimRotation = OwningCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(OwningCharacter->GetVelocity());
	
	// 归一化角度差到[-180, 180]范围，避免360度突变
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	
	// 使用插值平滑过渡，避免角度突变造成的动画抖动
	// 插值速度为6，值越大响应越快
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// ========== 【4. 身体倾斜（Lean）计算】 ==========
	
	// 根据角色旋转速度计算身体倾斜角度
	// 原理：计算上一帧和当前帧的旋转差值，除以时间得到每秒旋转角度
	// 这样当角色快速转身时，身体会有相应的倾斜效果
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = OwningCharacter->GetActorRotation();
	
	// 归一化旋转差值
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	
	// 计算每秒旋转角度（Yaw变化 / 时间间隔）
	const float Target = Delta.Yaw / DeltaSeconds;
	
	// 平滑插值到目标倾斜值，限制在[-90, 90]范围内
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	// ========== 【5. 瞄准偏移（Aim Offset）更新】 ==========
	
	// 从角色获取瞄准偏移值
	// AO_Yaw和AO_Pitch用于Aim Offset动画，让角色可以朝不同方向瞄准
	// 而不需要转身
	AO_Yaw = OwningCharacter->GetAO_Yaw();
	AO_Pitch = OwningCharacter->GetAO_Pitch();

	// ========== 【6. 武器IK（逆向动力学）设置】 ==========
	
	// 只有当角色装备了武器且武器和角色的网格体都有效时才进行IK计算
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && OwningCharacter->GetMesh())
	{
		// ---- 6.1 左手IK变换计算 ----
		// 目标：将左手正确放置到武器的左手握把位置
		// 方法：获取武器左手插槽的世界位置，转换为右手骨骼的相对空间
		
		// 获取武器上"LeftHandSocket"插槽的世界空间变换
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(
			FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		
		FVector OutPosition;
		FRotator OutRotation;
		
		// 将世界空间的位置和旋转转换为右手骨骼的局部空间
		// 这样LeftHandTransform就变成了相对于hand_r骨骼的偏移
		// 动画蓝图中的Two Bone IK节点会使用这个值
		OwningCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(),
		                                                 LeftHandTransform.GetRotation().Rotator(), OutPosition,
		                                                 OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// ---- 6.2 本地控制角色右手旋转同步 ----
		// 只有本地控制的角色才需要同步右手旋转
		// 这样武器的朝向会与准心完全对齐
		if (OwningCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			
			// 获取右手骨骼的世界空间变换
			FTransform RightHandTransform = OwningCharacter->GetMesh()->GetSocketTransform(
				FName("hand_r"), ERelativeTransformSpace::RTS_World);
			
			// 计算右手应该指向的旋转（朝向准心目标点）
			// 这里使用了向量反射的技巧来保持正确的武器朝向
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - OwningCharacter->
					GetHitTarget()));
			
			// 平滑插值到目标旋转，速度为30（较快的响应）
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}

		// ---- 6.3 FABRIK IK控制 ----
		// FABRIK用于手臂IK，在某些状态下需要禁用
		// 基础条件：只有空闲状态才启用FABRIK
		bUseFABRIK = OwningCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
		
		// 本地控制角色的额外条件
		// 在投掷手雷、切换武器时禁用FABRIK
		bool bFabrik = OwningCharacter->IsLocallyControlled() && 
			OwningCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && 
			OwningCharacter->GetCombatState() != ECombatState::ECS_SwappingWeapons &&
			OwningCharacter->bFinishedSwapping;
		
		// 如果满足基本条件，再检查是否在本地换弹
		// 换弹时也需要禁用FABRIK
		if (bFabrik)
		{
			bUseFABRIK = !OwningCharacter->IsLocallyReloading();
		}

		// ---- 6.4 瞄准偏移和右手变换控制 ----
		// 在换弹或禁用游戏玩法时禁用瞄准偏移
		bUseAimOffsets = OwningCharacter->GetCombatState() != ECombatState::ECS_Reloading && 
			!OwningCharacter->GetDisableGameplay();
		
		// 同步控制右手变换
		bTransformRightHand = OwningCharacter->GetCombatState() != ECombatState::ECS_Reloading && 
			!OwningCharacter->GetDisableGameplay();
		
		// 【调试代码】绘制枪口射线，用于调试瞄准方向
		// FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlashSocket"), ERelativeTransformSpace::RTS_World);
		// FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		// DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		// DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), OwningCharacter->GetHitTarget(), FColor::Orange);
	}
}
