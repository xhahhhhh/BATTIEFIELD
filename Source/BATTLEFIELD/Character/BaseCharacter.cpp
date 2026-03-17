#include "BaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "../Weapon/WeaponBase.h"
#include "../Components/CombatComponent.h"
#include "../Components/BuffComponent.h"
#include "BaseAnimInstance.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "../BATTLEFIELD.h"
#include "../PlayerController/BasePlayerController.h"
#include "../GameMode/PlayerGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "../PlayerState/BasePlayerState.h"
#include "../Weapon/WeaponTypes.h"
#include "BATTLEFIELD/Components/LagCompensationComponent.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/PlayerStart/TeamPlayerStart.h"
#include "Components/BoxComponent.h"

/*========================================
 * 构造函数与初始化
 *========================================*/

ABaseCharacter::ABaseCharacter()
{
	// 启用每帧Tick
	PrimaryActorTick.bCanEverTick = true;
	
	// 设置生成时的碰撞处理方式：尽可能调整位置，但总是生成
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	/*---- 相机系统设置 ----*/
	// 创建弹簧臂组件并附加到骨骼网格
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;           // 相机与角色的距离
	CameraBoom->bUsePawnControlRotation = true;     // 跟随控制器旋转

	// 创建跟随相机并附加到弹簧臂
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;  // 不跟随控制器旋转（由弹簧臂控制）

	/*---- UI组件 ----*/
	OverHeadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeadWidget"));
	OverHeadWidget->SetupAttachment(RootComponent);

	/*---- 游戏逻辑组件 ----*/
	// 战斗组件 - 处理武器、射击等
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	// 增益组件 - 处理加速、跳跃等Buff
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff"));
	Buff->SetIsReplicated(true);

	// 延迟补偿组件 - 服务器端命中验证
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));
	LagCompensation->SetIsReplicated(true);

	/*---- 移动设置 ----*/
	// 默认不使用控制器旋转Yaw（非战斗状态下）
	bUseControllerRotationYaw = false;
	// 角色朝向移动方向
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// 启用蹲下
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	// 转身速度
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);

	/*---- 碰撞设置 ----*/
	// 胶囊体忽略相机碰撞
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// 骨骼网格设置
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	/*---- 初始化状态 ----*/
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	/*---- 网络同步频率 ----*/
	// 每秒66次更新（约15ms间隔）
	SetNetUpdateFrequency(66.f);
	// 最小每秒33次更新
	SetMinNetUpdateFrequency(33.f);

	/*---- 溶解效果组件 ----*/
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	/*---- 手雷组件 ----*/
	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedGrenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("LeftHandSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/*---- 命中检测碰撞盒设置 ----*/
	// 创建身体各部位的碰撞盒用于服务器倒带命中检测
	// 头部
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("Head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);

	// 骨盆
	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	// 脊柱各节
	spine_01 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_01"));
	spine_01->SetupAttachment(GetMesh(), FName("spine_01"));
	HitCollisionBoxes.Add(FName("spine_01"), spine_01);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	spine_04 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_04"));
	spine_04->SetupAttachment(GetMesh(), FName("spine_04"));
	HitCollisionBoxes.Add(FName("spine_04"), spine_04);

	spine_05 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_05"));
	spine_05->SetupAttachment(GetMesh(), FName("spine_05"));
	HitCollisionBoxes.Add(FName("spine_05"), spine_05);

	// 手臂
	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	// 手部
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	// 腿部
	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	// 配置所有命中盒的碰撞属性
	for (auto& Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_Hitbox);           // 设置为命中盒类型
			Box.Value->SetCollisionResponseToAllChannels(ECR_Ignore);// 忽略所有通道
			Box.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block); // 只响应命中盒通道
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 默认禁用碰撞
		}
	}
}

/*========================================
 * 网络复制与组件初始化
 *========================================*/

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 重叠武器只对拥有者客户端同步（优化带宽）
	DOREPLIFETIME_CONDITION(ABaseCharacter, OverlappingWeapon, COND_OwnerOnly);
	// 生命值对所有客户端同步
	DOREPLIFETIME(ABaseCharacter, Health);
	// 护盾值对所有客户端同步
	DOREPLIFETIME(ABaseCharacter, Shield);
	// 禁用游戏玩法状态对所有客户端同步
	DOREPLIFETIME(ABaseCharacter, bDisableGameplay);
}

void ABaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 初始化Actor组件之间的引用关系
	// 战斗组件设置角色引用
	if (Combat)
	{
		Combat->Character = this;
	}
	
	// 增益组件初始化
	if (Buff)
	{
		Buff->Character = this;
		// 保存初始移动速度用于Buff效果计算
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		// 保存初始跳跃速度
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	
	// 延迟补偿组件初始化
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABasePlayerController>(Controller);
		}
	}
}

/*========================================
 * 动画蒙太奇播放方法
 *========================================*/

void ABaseCharacter::PlayFireMontage(bool bAiming)
{
	// 检查战斗组件和武器是否有效
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireMontage)
	{
		// 播放射击动画蒙太奇
		AnimInstance->Montage_Play(FireMontage);
		
		// 跳转到对应段落（目前瞄准和非瞄准使用同一段落）
		FName SectionName = bAiming ? FName("Rifle_Hip") : FName("Rifle_Hip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		// 播放淘汰（死亡）动画
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABaseCharacter::PlayReloadMontage()
{
	// 根据当前装备的武器类型播放对应的换弹动画
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		
		// 根据武器类型选择动画段落
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SMG:
			SectionName = FName("SMG");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		// 播放投掷手雷动画
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABaseCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		// 播放切换武器动画
		AnimInstance->Montage_Play(SwapMontage);
	}
}

/*========================================
 * 网络同步与淘汰系统
 *========================================*/

// 移动同步回调 - 当服务器同步移动数据到客户端时触发
void ABaseCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	// 触发模拟代理转身处理
	SimProxiesTurn();
	// 重置移动同步计时器
	TimeSinceLastMovementReplication = 0.f;
}

void ABaseCharacter::Elim(bool bPlayerLeftGame)
{
	// 服务器端执行：掉落或销毁武器，然后多播淘汰效果到所有客户端
	DropOrDestroyWeapons();
	MulticastElim(bPlayerLeftGame);
}

void ABaseCharacter::Destroyed()
{
	// 清理淘汰特效组件
	if (ElimEffectComponent)
	{
		ElimEffectComponent->DestroyComponent();
	}

	// 获取游戏模式并检查比赛状态
	PlayerGameMode = PlayerGameMode == nullptr ? GetWorld()->GetAuthGameMode<APlayerGameMode>() : PlayerGameMode;
	bool bMatchInProgress = PlayerGameMode && PlayerGameMode->GetMatchState() != MatchState::InProgress;

	// 比赛结束时销毁角色持有的武器
	if (Combat && Combat->EquippedWeapon && bMatchInProgress)
	{
		Combat->EquippedWeapon->Destroy();
		Combat->EquippedWeapon = nullptr;
	}
	Super::Destroyed();
}

// 多播RPC实现：在所有客户端上执行淘汰效果
void ABaseCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	
	// 更新HUD弹药为0
	if (BasePlayerController)
	{
		BasePlayerController->SetHUDWeaponAmmo(0);
	}
	
	// 设置淘汰标志
	bElimmed = true;
	
	// 播放淘汰动画
	PlayElimMontage();
	
	/* 溶解效果已禁用（注释掉）
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), .55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	*/

	// 禁用角色移动
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	
	// 停止射击
	if (Combat)
	{
		Combat->ShootPressed(false);
	}
	
	/* 禁用输入已注释掉
	if (BasePlayerController != nullptr)
	{
		DisableInput(BasePlayerController);
	}
	*/
	
	// 禁用所有碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 生成淘汰特效
	if (ElimEffect)
	{
		FVector ElimSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimEffectComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), ElimEffect, ElimSpawnPoint, GetActorRotation());
	}
	
	// 播放淘汰音效
	if (ElimSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, ElimSound, GetActorLocation());
	}
	
	// 如果正在使用狙击镜，关闭狙击镜UI
	bool bHideSniperScope = IsLocallyControlled() && Combat && Combat->bAiming && 
	                        Combat->EquippedWeapon && 
	                        Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	// 销毁皇冠组件（如果存在）
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}

	// 设置重生计时器
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABaseCharacter::ElimTimerFinished,
		ElimDelay
	);
}

// 淘汰计时器完成回调 - 请求重生或处理离开游戏
void ABaseCharacter::ElimTimerFinished()
{
	PlayerGameMode = PlayerGameMode == nullptr ? GetWorld()->GetAuthGameMode<APlayerGameMode>() : PlayerGameMode;
	
	// 如果比赛仍在进行且玩家未离开，请求重生
	if (PlayerGameMode && !bLeftGame)
	{
		PlayerGameMode->RequestRespawn(this, Controller);
	}
	
	// 如果玩家离开游戏，广播离开事件
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}

}

// 服务器RPC实现：处理玩家离开游戏
void ABaseCharacter::ServerLeaveGame_Implementation()
{
	PlayerGameMode = PlayerGameMode == nullptr ? GetWorld()->GetAuthGameMode<APlayerGameMode>() : PlayerGameMode;
	BasePlayerState = BasePlayerState == nullptr ? GetPlayerState<ABasePlayerState>() : BasePlayerState;
	
	if (PlayerGameMode && BasePlayerState)
	{
		PlayerGameMode->PlayerLeftGame(BasePlayerState);
	}
}

/*========================================
 * 武器处理方法
 *========================================*/

void ABaseCharacter::DropOrDestroyWeapon(AWeaponBase* Weapon)
{
	if (Weapon == nullptr) return;
	
	// 根据武器标志决定是销毁还是掉落
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();  // 销毁武器（如默认武器）
	}
	else
	{
		Weapon->Dropped();  // 掉落武器到地面
	}
}

void ABaseCharacter::DropOrDestroyWeapons()
{
	// 淘汰时处理所有武器
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

/*========================================
 * 伤害与受击系统
 *========================================*/

void ABaseCharacter::PlayHitReactMontage()
{
	// 检查战斗组件和武器是否有效
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		// 播放受击反应动画
		AnimInstance->Montage_Play(HitReactMontage);
		
		// 使用"FromLeft"段落
		FName SectionName("FromLeft");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

// 接收伤害回调 - 绑定到OnTakeAnyDamage委托
void ABaseCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                   AController* InstigatorController, AActor* DamageCauser)
{
	// 获取游戏模式用于伤害计算
	PlayerGameMode = PlayerGameMode == nullptr ? GetWorld()->GetAuthGameMode<APlayerGameMode>() : PlayerGameMode;
	
	// 如果已淘汰或游戏模式无效，忽略伤害
	if (bElimmed || PlayerGameMode == nullptr) return;
	
	// 通过游戏模式计算最终伤害（可能考虑友军伤害、距离衰减等）
	Damage = PlayerGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	// 护盾吸收伤害逻辑
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			// 护盾足够吸收所有伤害
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			// 护盾耗尽，剩余伤害转嫁到生命值
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}

	// 应用伤害到生命值
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
	
	// 更新HUD显示
	UpdateHUDHealth();
	UpdateHUDShield();
	
	// 播放受击动画
	PlayHitReactMontage();
	
	// 生命值归零，执行淘汰
	if (Health <= 0.f)
	{
		if (PlayerGameMode)
		{
			BasePlayerController = BasePlayerController == nullptr
				                       ? Cast<ABasePlayerController>(Controller)
				                       : BasePlayerController;
			ABasePlayerController* AttackerController = Cast<ABasePlayerController>(InstigatorController);
			PlayerGameMode->PlayerEliminated(this, BasePlayerController, AttackerController);
		}
	}
}

/*========================================
 * 玩家状态与重生系统
 *========================================*/

// 轮询初始化 - 每帧检查PlayerState是否可用
void ABaseCharacter::PollInit()
{
	if (BasePlayerState == nullptr)
	{
		BasePlayerState = GetPlayerState<ABasePlayerState>();
		if (BasePlayerState)
		{
			OnPlayerStateInitialized();
			
			// 检查是否是领先玩家，如果是则播放皇冠特效
			ABaseGameState* BaseGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
			if (BaseGameState && BaseGameState->TopScoringPlayers.Contains(BasePlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

// 玩家状态初始化完成回调
void ABaseCharacter::OnPlayerStateInitialized()
{
	// 初始化分数显示（触发HUD更新）
	BasePlayerState->AddToScore(0.f);
	BasePlayerState->AddToDefeats(0);
	
	// 根据团队设置材质颜色
	SetTeamColor(BasePlayerState->GetTeam());
	
	// 设置重生点
	SetSpawnPoint();
}

// 设置重生点 - 在所属团队的出生点中随机选择
void ABaseCharacter::SetSpawnPoint()
{
	// 仅在服务器执行且玩家已分配团队
	if (HasAuthority() && BasePlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		
		// 筛选出同团队的出生点
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == BasePlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}
		
		// 随机选择一个出生点并设置位置和旋转
		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
		}
	}
}

/*========================================
 * 生命周期方法
 *========================================*/

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 生成默认武器
	SpawnDefaultWeapon();
	
	// 初始化HUD显示
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	
	// 服务器端绑定伤害回调
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage);
	}
	
	// 初始隐藏手雷
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}

	/* 本地控制端伤害处理已注释
	if (IsLocallyControlled())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseCharacter::ReceiveDamage);
	}
	*/
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 处理原地转身和瞄准偏移
	RotateInPlace(DeltaTime);
	
	// 相机距离过近时隐藏角色
	HideCameraIfCharacterClose();
	
	// 轮询初始化PlayerState
	PollInit();
}

/*========================================
 * 瞄准偏移与转身系统
 *========================================*/

// 在原地旋转角色 - 处理不同状态下的旋转逻辑
void ABaseCharacter::RotateInPlace(float DeltaTime)
{
	// 夺旗模式：持有旗帜时面向移动方向
	if (Combat && Combat->bHoldTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	// 装备武器时：使用控制器旋转（瞄准模式）
	if (Combat && Combat->EquippedWeapon)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
	
	// 游戏玩法禁用时（如淘汰后）：停止旋转
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	// 本地控制的自主代理：计算瞄准偏移
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		// 模拟代理：定期触发转身检测
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > .25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

/*========================================
 * 输入系统设置
 *========================================*/

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 添加增强输入映射上下文
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}

	// 绑定输入动作到回调函数
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 移动和视角
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);
		EnhancedInputComponent->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
		
		// 基础动作
		EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ABaseCharacter::Jump);
		EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ABaseCharacter::DoCrouch);
		
		// 武器相关
		EnhancedInputComponent->BindAction(IA_Equip, ETriggerEvent::Started, this, &ABaseCharacter::EquipButtonPressed);
		EnhancedInputComponent->BindAction(IA_Shoot, ETriggerEvent::Started, this, &ABaseCharacter::ShootPressed);
		EnhancedInputComponent->BindAction(IA_Shoot, ETriggerEvent::Completed, this, &ABaseCharacter::ShootReleased);
		EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started, this, &ABaseCharacter::ReloadPressed);
		EnhancedInputComponent->BindAction(IA_SwapWeapon, ETriggerEvent::Started, this, &ABaseCharacter::SwapWeapon);
		
		// 瞄准
		EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Started, this, &ABaseCharacter::Aim);
		EnhancedInputComponent->BindAction(IA_Aim, ETriggerEvent::Completed, this, &ABaseCharacter::AimEnd);
		
		// 投掷
		EnhancedInputComponent->BindAction(IA_Throw, ETriggerEvent::Started, this, &ABaseCharacter::ThrowPressed);
	}
}

/*========================================
 * 输入动作回调函数
 *========================================*/

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	// 游戏玩法禁用时忽略输入
	if (bDisableGameplay) return;

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (GetController() != nullptr)
	{
		// 获取控制器的Yaw旋转（只使用水平方向）
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 计算前后方向向量
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// 计算左右方向向量
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 应用移动输入
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABaseCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (GetController() != nullptr)
	{
		// X轴控制Yaw（水平旋转）
		AddControllerYawInput(LookAxisVector.X);
		// Y轴控制Pitch（垂直旋转）
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABaseCharacter::DoJump()
{
	if (bDisableGameplay) return;
	Jump();
}

void ABaseCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		/* 旧的服务器授权代码（已注释）
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
		*/
		
		// 夺旗模式下不能装备武器
		if (Combat->bHoldTheFlag) return;
		
		// 只有在空闲状态下才能装备武器
		if (Combat->CombatState == ECombatState::ECS_Unoccupied)
			ServerEquipButtonPressed();
	}
}

void ABaseCharacter::DoCrouch()
{
	// 夺旗模式下不能蹲下
	if (Combat->bHoldTheFlag) return;
	if (bDisableGameplay) return;
	
	// 切换蹲下状态
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABaseCharacter::ReloadPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void ABaseCharacter::Aim()
{
	// 夺旗模式下不能瞄准
	if (Combat->bHoldTheFlag) return;
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABaseCharacter::AimEnd()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABaseCharacter::Jump()
{
	// 蹲下时先取消蹲下
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABaseCharacter::ShootPressed()
{
	// 夺旗模式下不能射击
	if (Combat->bHoldTheFlag) return;
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->ShootPressed(true);
	}
}

void ABaseCharacter::ShootReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->ShootPressed(false);
	}
}

void ABaseCharacter::ThrowPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ABaseCharacter::SwapWeapon()
{
	// 检查是否可以切换武器
	bool bSwap = Combat && Combat->ShouldSwapWeapons() && 
	             Combat->CombatState == ECombatState::ECS_Unoccupied && 
	             OverlappingWeapon == nullptr;
	if (bSwap)
	{
		PlaySwapMontage();
		Combat->CombatState = ECombatState::ECS_SwappingWeapons;
		bFinishedSwapping = false;
		ServerSwapWeapon();
	}
}


float ABaseCharacter::CalculateSpeed()
{
	// 计算水平面速度（忽略Z轴）
	return GetVelocity().Size2D();
}

// 计算瞄准偏移（Aim Offset）用于混合空间动画
void ABaseCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	float Speed = CalculateSpeed();
	
	// 站立且不在空中：启用根骨骼旋转和转身
	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		
		// 计算当前瞄准旋转与起始旋转的差值
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		
		// 初始化插值Yaw
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	
	// 移动中或在空中：禁用根骨骼旋转
	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABaseCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	
	// 非本地控制的代理需要转换Pitch范围
	// 因为网络同步将[-90,0]压缩到[270,360]
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// 将Pitch从[270,360]映射回[-90,0]
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

// 模拟代理转身处理 - 用于网络同步的其他玩家角色
void ABaseCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	
	// 移动时不处理转身
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	// 计算帧间旋转变化
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	// 根据旋转变化决定转身方向
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

// 原地转身逻辑 - 用于本地控制的平滑转身
void ABaseCharacter::TurnInPlace(float DeltaTime)
{
	// 根据AO_Yaw决定转身方向
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	
	// 正在转身时：插值平滑过渡
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 2.f);
		AO_Yaw = InterpAO_Yaw;
		
		// 转身接近完成时重置状态
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

/*========================================
 * 相机与HUD系统
 *========================================*/

// 当相机距离角色过近时隐藏角色网格（防止穿模）
void ABaseCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		// 相机过近：隐藏角色和武器
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		// 相机距离足够：显示角色和武器
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
		if (Combat && Combat->SecondaryWeapon && Combat->SecondaryWeapon->GetWeaponMesh())
		{
			Combat->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

// 更新HUD生命值显示
void ABaseCharacter::UpdateHUDHealth()
{
	BasePlayerController = BasePlayerController == nullptr
		                       ? Cast<ABasePlayerController>(GetController())
		                       : BasePlayerController;
	if (BasePlayerController)
	{
		BasePlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

// 更新HUD护盾值显示
void ABaseCharacter::UpdateHUDShield()
{
	BasePlayerController = BasePlayerController == nullptr
		                       ? Cast<ABasePlayerController>(GetController())
		                       : BasePlayerController;
	if (BasePlayerController)
	{
		BasePlayerController->SetHUDShield(Shield, MaxShield);
	}
}

// 更新HUD弹药显示
void ABaseCharacter::UpdateHUDAmmo()
{
	BasePlayerController = BasePlayerController == nullptr
		                       ? Cast<ABasePlayerController>(GetController())
		                       : BasePlayerController;
	if (BasePlayerController && Combat && Combat->EquippedWeapon)
	{
		BasePlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BasePlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

/*========================================
 * 武器与团队系统
 *========================================*/

// 生成默认武器
void ABaseCharacter::SpawnDefaultWeapon()
{
	PlayerGameMode = PlayerGameMode == nullptr ? GetWorld()->GetAuthGameMode<APlayerGameMode>() : PlayerGameMode;
	UWorld* World = GetWorld();
	
	if (PlayerGameMode && World && !bElimmed && DefaultWeaponClass)
	{
		// 生成默认武器
		AWeaponBase* StartingWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true; // 标记为需要销毁的武器（非掉落）
		
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

// 根据团队设置角色材质颜色
void ABaseCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr || OriginalMaterial == nullptr) return;
	
	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		DissolveMaterialInstance = BlueDissolveMatInst;
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		DissolveMaterialInstance = RedDissolveMatInst;
		break;
	}
}

/*========================================
 * 皇冠特效与属性复制回调
 *========================================*/

// 多播实现：获得领先位置时播放皇冠特效
void ABaseCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr) return;
	
	// 首次创建皇冠组件
	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.0f, 0.0f, 150.f), // 头顶上方150单位
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
	
	// 激活特效
	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

// 多播实现：失去领先位置时销毁皇冠特效
void ABaseCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

// 生命值复制回调
void ABaseCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	// 生命值减少时播放受击动画
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

// 护盾值复制回调
void ABaseCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	// 护盾减少时播放受击动画
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

/*========================================
 * 溶解效果系统
 *========================================*/

// 更新溶解材质参数
void ABaseCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue("DissolveValue", DissolveValue);
	}
}

// 开始溶解动画
void ABaseCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABaseCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

/*========================================
 * 武器访问器方法
 *========================================*/

void ABaseCharacter::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	// 隐藏上一个重叠武器的拾取提示
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon = Weapon;
	
	// 本地控制端显示新重叠武器的拾取提示
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABaseCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABaseCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeaponBase* ABaseCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ABaseCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	if (Combat->HitTarget == FVector::ZeroVector) return Combat->End;
	return Combat->HitTarget;
}

/*========================================
 * 服务器RPC实现
 *========================================*/

// 服务器RPC实现：装备武器
void ABaseCharacter::ServerEquipButtonPressed_Implementation()
{
	// 只在服务器执行
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
	}
}

// 服务器RPC实现：切换武器
void ABaseCharacter::ServerSwapWeapon_Implementation()
{
	if (Combat && Combat->ShouldSwapWeapons())
	{
		Combat->SwapWeapons();
	}
}

// 重叠武器复制回调
void ABaseCharacter::OnRep_OverlappingWeapon(AWeaponBase* Lastweapon)
{
	// 显示新武器拾取提示
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	// 隐藏上一个武器的拾取提示
	if (Lastweapon)
	{
		Lastweapon->ShowPickupWidget(false);
	}
}

/*========================================
 * 状态查询方法
 *========================================*/

ECombatState ABaseCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

bool ABaseCharacter::IsLocallyReloading()
{
	if (Combat == nullptr) return false;
	return Combat->bLocallyReloading;
}

bool ABaseCharacter::IsHoldingTheFlag() const
{
	if (Combat == nullptr) return false;
	return Combat->bHoldTheFlag;
}

ETeam ABaseCharacter::GetTeam()
{
	BasePlayerState = BasePlayerState == nullptr ? GetPlayerState<ABasePlayerState>() : BasePlayerState;
	if (BasePlayerState == nullptr) return ETeam::ET_NoTeam;
	return BasePlayerState->GetTeam();
}

void ABaseCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (Combat == nullptr) return;
	Combat->bHoldTheFlag = bHolding;
}
