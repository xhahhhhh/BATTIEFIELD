#include "CombatComponent.h"
#include "../Weapon/WeaponBase.h"
#include "../Character/BaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "../PlayerController/BasePlayerController.h"
//#include "../HUD/PlayerHUD.h"
#include <gsl/pointers>

#include "camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "BATTLEFIELD//Character/BaseAnimInstance.h"
#include "BATTLEFIELD/Weapon/Projectile.h"
#include "BATTLEFIELD/Weapon/Shotgun.h"

/**
 * 构造函数
 * 初始化移动速度和组件Tick
 */
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
}

/**
 * 拾取弹药
 * 增加对应武器类型的携带弹药，如果当前装备的武器已空且类型匹配则自动换弹
 * 
 * @param WeaponType - 武器类型
 * @param AmmoAmount - 弹药数量
 */
void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);
		UpdateCarriedAmmo();
	}
	// 如果当前装备的武器已空且类型匹配，自动换弹
	if (EquippedWeapon && EquippedWeapon->IsEmpty() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		Reload();
	}
}

/**
 * 组件开始播放
 * 初始化移动速度、FOV、弹药（仅服务器）
 */
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		// 只在服务器初始化弹药
		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

/**
 * 每帧Tick函数
 * 
 * 本地控制的角色执行：
 * - 准心射线检测
 * - 更新HUD准心
 * - FOV插值（瞄准效果）
 */
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		SetHUDCrossHairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

/**
 * 设置HUD准心
 * 
 * 计算准心扩散程度，考虑以下因素：
 * - 移动速度：移动越快扩散越大
 * - 跳跃状态：空中扩散最大
 * - 瞄准状态：瞄准缩小准心
 * - 射击后坐力：射击后准心暂时扩大
 * 
 * @param DeltaTime - 帧间隔
 */
void UCombatComponent::SetHUDCrossHairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<APlayerHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			// 设置准心纹理
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsButtom = EquippedWeapon->CrosshairsButtom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsButtom = nullptr;
			}
			
			// 根据移动速度计算准心扩散因子
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange,
			                                                            Velocity.Size());

			// 空中状态增加扩散
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			// 瞄准状态缩小准心
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			// 射击后坐力逐渐恢复
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			// 计算最终准心扩散值
			HUDPackage.CrosshairSpread = .5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor +
				CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

/**
 * FOV插值（瞄准缩放效果）
 * 
 * 在默认FOV和瞄准FOV之间平滑过渡
 * @param DeltaTime - 帧间隔
 */
void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, ZoomedFOV, DeltaTime, EquippedWeapon->ZoomInterpSpeed);
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

/**
 * 开始射击计时器
 * 
 * 控制武器射速，防止过快射击
 */
void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

/**
 * 射击计时结束
 * 
 * 恢复射击能力，如果是自动武器且按键仍按下则继续射击
 */
void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;

	bCanFire = true;
	if (bShootPressed && EquippedWeapon->bAutomatic)
	{
		Shoot();
	}
	ReloadEmptyWeapon();
}

/**
 * 注册网络同步属性
 */
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME(UCombatComponent, Grenades);
	DOREPLIFETIME(UCombatComponent, bHoldTheFlag);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
}

/**
 * 设置瞄准状态
 * 
 * 本地立即响应，同时通知服务器同步给其他客户端
 * @param bIsAiming - 是否瞄准
 */
void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	// 狙击枪显示瞄准镜UI
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
	if (Character->IsLocallyControlled()) bAimButtonPressed = bIsAiming;
}

/**
 * 服务器RPC - 设置瞄准状态
 * @param bIsAiming - 是否瞄准
 */
void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

/**
 * RepNotify - 装备武器变化
 * 
 * 所有客户端（包括非本地控制）执行：
 * - 设置武器状态
 * - 附加到右手
 * - 设置移动方向
 * - 播放音效
 */
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
		PlayEquipSound(EquippedWeapon);
		EquippedWeapon->EnableCustomDepth(false);
		EquippedWeapon->SetHUDAmmo();
	}
}

/**
 * RepNotify - 副武器变化
 * 
 * 副武器附加到背包位置
 */
void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToBackpack(SecondaryWeapon);
		PlayEquipSound(SecondaryWeapon);
	}
}

/**
 * 射击按键处理
 * @param bPressed - 是否按下
 */
void UCombatComponent::ShootPressed(bool bPressed)
{
	bShootPressed = bPressed;
	if (bShootPressed)
	{
		Shoot();
	}
}

/**
 * 动画通知：霰弹枪装填单发弹药
 * 
 * 服务器更新弹药，逐发装填
 */
void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

/**
 * 跳转到霰弹枪换弹结束动画
 * 
 * 当弹匣已满或弹药耗尽时提前结束换弹动画
 */
void UCombatComponent::JumpToShotgunEnd()
{
	if (EquippedWeapon->IsFull())
	{
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance && Character->GetReloadMontage())
		{
			AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
		}
	}
}

/**
 * 动画通知：手雷投掷结束
 */
void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
}

/**
 * 动画通知：发射手雷
 * 
 * 本地控制的角色通知服务器生成手雷抛射物
 */
void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

/**
 * 服务器RPC - 发射手雷
 * 
 * 服务器生成手雷抛射物，计算投掷方向和速度
 * @param Target - 目标位置
 */
void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(GrenadeClass, StartingLocation, ToTarget.Rotation(), SpawnParams);
		}
	}
}

/**
 * 丢弃当前装备武器
 */
void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

/**
 * 附加 Actor 到右手插槽
 * @param ActorToAttach - 要附加的Actor
 */
void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

/**
 * 附加 Actor 到左手插槽
 * 手枪和冲锋枪使用专门的PistolSocket
 * @param ActorToAttach - 要附加的Actor
 */
void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon ==
		nullptr)
		return;
	bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || EquippedWeapon->
		GetWeaponType() == EWeaponType::EWT_SMG;
	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

/**
 * 附加 Actor 到背包插槽
 * @param ActorToAttach - 要附加的Actor
 */
void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

/**
 * 附加旗帜到右手插槽
 * @param Flag - 旗帜武器
 */
void UCombatComponent::AttachFlagToRightHand(AWeaponBase* Flag)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || Flag == nullptr) return;
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("FlagSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(Flag, Character->GetMesh());
	}
}

/**
 * 更新携带弹药显示
 */
void UCombatComponent::UpdateCarriedAmmo()
{
	if (EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

/**
 * 播放装备武器音效
 * @param WeaponToEquip - 要装备的武器
 */
void UCombatComponent::PlayEquipSound(AWeaponBase* WeaponToEquip)
{
	if (Character && WeaponToEquip && WeaponToEquip->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponToEquip->EquipSound, Character->GetActorLocation());
	}
}

/**
 * 空弹匣时自动换弹
 */
void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

/**
 * 执行射击
 * 
 * 检查射击条件，根据武器类型调用对应的射击函数
 */
void UCombatComponent::Shoot()
{
	if (CanFire())
	{
		bCanFire = false;
		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 0.5f;

			// 根据武器射击类型选择对应函数
			switch (EquippedWeapon->FireType)
			{
			case EFireType::EFT_Projectile:
				ShootProjectileWeapon();
				break;
			case EFireType::EFT_HitScan:
				ShootHitScanWeapon();
				break;
			case EFireType::EFT_Shotgun:
				ShootShotgun();
				break;
			}
		}
		StartFireTimer();
	}
}

/**
 * 抛射物武器射击
 * 
 * 处理散射逻辑，本地立即显示效果，同时通知服务器
 */
void UCombatComponent::ShootProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		// 如果启用散射，计算散射后的目标点
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		// 非权威客户端本地立即显示效果（预测）
		if (!Character->HasAuthority()) LocalShoot(HitTarget);
		// 通知服务器
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

/**
 * 即时命中武器射击
 * 
 * 处理散射逻辑，本地立即显示效果，同时通知服务器
 */
void UCombatComponent::ShootHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->bUseScatter ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority()) LocalShoot(HitTarget);
		ServerFire(HitTarget, EquippedWeapon->FireDelay);
	}
}

/**
 * 霰弹枪射击
 * 
 * 霰弹枪特殊处理：一次射击多个弹丸，每个弹丸可能有不同的目标点
 */
void UCombatComponent::ShootShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		// 计算多个弹丸的散射目标
		Shotgun->ShotgunTraceEndWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority()) LocalShotgunShoot(HitTargets);
		ServerShotgunFire(HitTargets, EquippedWeapon->FireDelay);
	}
}

/**
 * 准心射线检测
 * 
 * 从屏幕中心发射射线，检测准心指向的位置
 * 用于确定射击方向和目标
 * 
 * @param TraceHitResult - 检测结果
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// 获取屏幕中心坐标
	FVector2D CrossHairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	// 将屏幕坐标转换为世界坐标和方向
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrossHairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrossHairWorldPosition;

		if (Character)
		{
			// 从角色前方开始检测，避免太近检测不到
			float DistacnceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDirection * (DistacnceToCharacter + 100.f);
		}

		// 计算射线终点
		End = Start + CrossHairWorldDirection * TRACE_LENGTH;

		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
			HitTarget = End;
		}

		// 执行射线检测
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
		);
		
		// 根据检测到的Actor类型改变准心颜色
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

/**
 * 服务器RPC - 开火（实现）
 * @param TraceHitTarget - 目标位置
 */
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	MulticastFire(TraceHitTarget);
}

/**
 * 服务器RPC - 开火（验证）
 * 
 * 验证射击间隔是否合法，防止作弊
 * @param TraceHitTarget - 目标位置
 * @param FireDelay - 客户端报告的射击间隔
 * @return 验证是否通过
 */
bool UCombatComponent::ServerFire_Validate(const FVector_NetQuantize& TraceHitTarget, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

/**
 * 多播RPC - 开火
 * 
 * 同步射击效果给所有客户端
 * 本地控制的客户端已经预测执行过，跳过
 * 
 * @param TraceHitTarget - 目标位置
 */
void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 本地控制的客户端跳过（已经预测执行）
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalShoot(TraceHitTarget);
}

/**
 * 服务器RPC - 霰弹枪开火（实现）
 * @param TraceHitTargets - 多个弹丸目标
 */
void UCombatComponent::ServerShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets,
                                                        float FireDelay)
{
	MulticastShotgunFire(TraceHitTargets);
}

/**
 * 服务器RPC - 霰弹枪开火（验证）
 * @param TraceHitTargets - 多个弹丸目标
 * @param FireDelay - 射击间隔
 * @return 验证是否通过
 */
bool UCombatComponent::ServerShotgunFire_Validate(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay)
{
	if (EquippedWeapon)
	{
		bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->FireDelay, FireDelay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

/**
 * 多播RPC - 霰弹枪开火
 * @param TraceHitTargets - 多个弹丸目标
 */
void UCombatComponent::MulticastShotgunFire_Implementation(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalShotgunShoot(TraceHitTargets);
}

/**
 * 本地射击执行
 * 
 * 实际执行射击逻辑：播放动画、调用武器射击
 * @param TraceHitTarget - 目标位置
 */
void UCombatComponent::LocalShoot(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

/**
 * 本地霰弹枪射击执行
 * @param TraceHitTargets - 多个弹丸目标
 */
void UCombatComponent::LocalShotgunShoot(const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		bLocallyReloading = false;
		Character->PlayFireMontage(bAiming);
		Shotgun->FireShotgun(TraceHitTargets);
	}
}

/**
 * 装备武器
 * 
 * 主入口函数，根据武器类型决定如何处理：
 * - 旗帜：特殊处理
 * - 其他武器：分为主武器和副武器
 * 
 * @param WeaponToEquip - 要装备的武器
 */
void UCombatComponent::EquipWeapon(AWeaponBase* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// 旗帜特殊处理
	if (WeaponToEquip->GetWeaponType() == EWeaponType::EWT_Flag)
	{
		bHoldTheFlag = true;
		WeaponToEquip->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachFlagToRightHand(WeaponToEquip);
		WeaponToEquip->SetOwner(Character);
		TheFlag = WeaponToEquip;
	}
	else
	{
		// 普通武器：决定是装备为主武器还是副武器
		if (EquippedWeapon != nullptr && SecondaryWeapon == nullptr)
		{
			EquipSecondaryWeapon(WeaponToEquip);
		}
		else
		{
			EquipPrimaryWeapon(WeaponToEquip);
		}
		EquippedWeapon->ShowPickupWidget(false);
		EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

/**
 * 切换主副武器
 */
void UCombatComponent::SwapWeapons()
{
	if (CombatState != ECombatState::ECS_Unoccupied || Character == nullptr) return;
	Character->PlaySwapMontage();
	Character->bFinishedSwapping = false;
	CombatState = ECombatState::ECS_SwappingWeapons;
	if (SecondaryWeapon) SecondaryWeapon->EnableCustomDepth(false);
}

/**
 * 装备主武器
 * @param WeaponToEquip - 要装备的武器
 */
void UCombatComponent::EquipPrimaryWeapon(AWeaponBase* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;
	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquipSound(WeaponToEquip);
	ReloadEmptyWeapon();
}

/**
 * 装备副武器
 * @param WeaponToEquip - 要装备的武器
 */
void UCombatComponent::EquipSecondaryWeapon(AWeaponBase* WeaponToEquip)
{
	if (WeaponToEquip == nullptr) return;
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(WeaponToEquip);
	PlayEquipSound(WeaponToEquip);
}

/**
 * RepNotify - 瞄准状态变化
 * 
 * 本地控制的角色需要特殊处理，避免覆盖本地输入
 */
void UCombatComponent::OnRep_Aiming()
{
	if (Character && Character->IsLocallyControlled())
	{
		bAiming = bAimButtonPressed;
	}
}

/**
 * 换弹
 * 
 * 检查条件后通知服务器执行换弹
 */
void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->IsFull() &&
		!bLocallyReloading)
	{
		ServerReload();
		HandleReload();
		bLocallyReloading = true;
	}
}

/**
 * 动画通知：换弹结束
 * 
 * 服务器更新弹药状态
 */
void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	bLocallyReloading = false;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	// 换弹期间按下了射击键，自动继续射击
	if (bShootPressed)
	{
		Shoot();
	}
}

/**
 * 动画通知：武器切换结束
 */
void UCombatComponent::FinishSwap()
{
	if (Character && Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
	}
	if (Character) Character->bFinishedSwapping = true;
}

/**
 * 动画通知：切换武器附加位置
 * 
 * 实际执行主副武器的交换
 */
void UCombatComponent::FinishSwapAttachWeapons()
{
	PlayEquipSound(EquippedWeapon);

	if (Character == nullptr || !Character->HasAuthority()) return;
	
	// 交换主副武器
	AWeaponBase* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;

	// 更新新主武器
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();

	// 更新新副武器
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
}

/**
 * 服务器RPC - 换弹
 */
void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_Reloading;
	// 非本地控制的客户端执行换弹动画
	if (!Character->IsLocallyControlled()) HandleReload();
}

/**
 * RepNotify - 战斗状态变化
 * 
 * 根据状态执行相应操作
 */
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		// 非本地控制的客户端播放换弹动画
		if (Character && !Character->IsLocallyControlled()) HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		// 空闲状态下如果按下了射击键，自动射击
		if (bShootPressed)
		{
			Shoot();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		// 非本地控制的客户端播放投掷手雷动画
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlayThrowGrenadeMontage();
			ShowAttachedGrenade(true);
		}
		break;
	case ECombatState::ECS_SwappingWeapons:
		// 非本地控制的客户端播放切换武器动画
		if (Character && !Character->IsLocallyControlled())
		{
			Character->PlaySwapMontage();
		}
		break;
	}
}

/**
 * 更新弹药数值
 * 
 * 从携带弹药中扣除，添加到武器弹匣
 */
void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}

/**
 * 更新霰弹枪弹药
 * 
 * 霰弹枪逐发装填，每次只加一发
 */
void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(1);
	bCanFire = true;
	// 弹匣满或弹药耗尽时结束换弹
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

/**
 * RepNotify - 手雷数量变化
 */
void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}

/**
 * 更新HUD手雷显示
 */
void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

/**
 * 是否可以切换武器
 * @return 有主武器和副武器时返回true
 */
bool UCombatComponent::ShouldSwapWeapons()
{
	return (EquippedWeapon != nullptr && SecondaryWeapon != nullptr);
}

/**
 * 处理换弹动画
 */
void UCombatComponent::HandleReload()
{
	if (Character)
		Character->PlayReloadMontage();
}

/**
 * 计算需要装填的弹药数量
 * @return 装填数量（取弹匣剩余空间和携带弹药的最小值）
 */
int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

/**
 * 投掷手雷
 * 
 * 本地立即响应，同时通知服务器
 */
void UCombatComponent::ThrowGrenade()
{
	if (Grenades == 0) return;
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		ShowAttachedGrenade(true);
	}
	// 非权威客户端通知服务器
	if (Character && !Character->HasAuthority())
	{
		Server_ThrowGrenade();
	}
	// 权威端直接更新
	if (Character && Character->HasAuthority())
	{
		Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
		UpdateHUDGrenades();
	}
}

/**
 * 服务器RPC - 投掷手雷
 */
void UCombatComponent::Server_ThrowGrenade_Implementation()
{
	if (Grenades == 0) return;
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		ShowAttachedGrenade(true);
	}
	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

/**
 * 显示/隐藏附加的手雷模型
 * @param bShowGrenade - 是否显示
 */
void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

/**
 * 检查是否可以射击
 * 
 * 检查条件：
 * - 有武器
 * - 武器有弹药或正在换弹的霰弹枪
 * - 射击间隔已过
 * - 战斗状态允许
 * 
 * @return 是否可以射击
 */
bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	// 霰弹枪可以在换弹期间射击（中断换弹）
	if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->
		GetWeaponType() == EWeaponType::EWT_Shotgun)
		return true;
	if (bLocallyReloading) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

/**
 * RepNotify - 携带弹药变化
 */
void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// 霰弹枪换弹时弹药耗尽，结束换弹动画
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo == 0;
	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

/**
 * 初始化携带弹药
 * 
 * 为每种武器类型设置初始弹药数量
 */
void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotGunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}

/**
 * RepNotify - 持旗状态变化
 */
void UCombatComponent::OnRep_HoldingTheFlag()
{
	if (bHoldTheFlag && Character && Character->IsLocallyControlled())
	{
		// 可在此添加持旗时的特殊处理
	}
}
