#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/PlayerHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../CharacterTypes/CombatState.h"
#include "CombatComponent.generated.h"

class ABaseCharacter;
class AWeaponBase;

/**
 * 战斗组件
 * 
 * 功能说明：
 * 管理角色的所有战斗相关功能，包括武器装备、射击、换弹、瞄准、手雷投掷等。
 * 处理三种射击类型：抛射物武器、即时命中武器、霰弹枪。
 * 
 * 网络架构：
 * - 本地预测：客户端立即响应玩家输入（射击、换弹等）
 * - 服务器验证：关键操作通过RPC发送到服务器验证
 * - 状态同步：使用RepNotify同步武器状态、弹药数量等
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	UCombatComponent();

	/**
	 * 每帧Tick函数
	 * 处理准心检测、HUD更新、FOV插值
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 注册需要网络同步的属性
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	friend class ABaseCharacter;

	/**
	 * 装备武器
	 * 主入口函数，根据武器类型决定是装备主武器、副武器还是旗帜
	 * @param WeaponToEquip - 要装备的武器
	 */
	void EquipWeapon(AWeaponBase* WeaponToEquip);

	/** 切换主副武器 */
	void SwapWeapons();

	/** 开始换弹 */
	void Reload();

	/** 
	 * 动画通知：换弹结束（蓝图可调用）
	 * 服务器更新弹药数量，客户端恢复战斗状态
	 */
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	/**
	 * 动画通知：武器切换结束（蓝图可调用）
	 */
	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	/**
	 * 动画通知：切换武器附加位置（蓝图可调用）
	 * 实际执行主副武器的位置交换
	 */
	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();

	/**
	 * 射击按键处理
	 * @param bPressed - 是否按下
	 */
	void ShootPressed(bool bPressed);

	/**
	 * 动画通知：霰弹枪装填单发弹药（蓝图可调用）
	 * 霰弹枪特殊换弹逻辑，逐发装填
	 */
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	/** 跳转到霰弹枪换弹结束动画 */
	void JumpToShotgunEnd();

	/**
	 * 动画通知：手雷投掷结束（蓝图可调用）
	 */
	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	/**
	 * 动画通知：发射手雷（蓝图可调用）
	 * 在手雷动画的特定帧调用，实际生成抛射物
	 */
	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	/**
	 * 服务器RPC - 发射手雷
	 * @param Target - 投掷目标位置
	 */
	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	/**
	 * 拾取弹药
	 * @param WeaponType - 武器类型
	 * @param AmmoAmount - 弹药数量
	 */
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	/** 本地是否正在换弹（用于本地预测） */
	bool bLocallyReloading = false;

protected:
	/** 组件开始播放 */
	virtual void BeginPlay() override;

	/**
	 * 设置瞄准状态
	 * 本地立即响应，同时通知服务器
	 * @param bIsAiming - 是否瞄准
	 */
	void SetAiming(bool bIsAiming);

	/**
	 * 服务器RPC - 设置瞄准状态
	 * 同步给其他客户端
	 */
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	/**
	 * RepNotify - 装备武器变化
	 * 所有客户端执行武器附加、动画设置等
	 */
	UFUNCTION()
	void OnRep_EquippedWeapon();

	/**
	 * RepNotify - 副武器变化
	 */
	UFUNCTION()
	void OnRep_SecondaryWeapon();

	/** 执行射击 */
	void Shoot();

	/** 抛射物武器射击 */
	void ShootProjectileWeapon();

	/** 即时命中武器射击 */
	void ShootHitScanWeapon();

	/** 霰弹枪射击 */
	void ShootShotgun();

	/**
	 * 本地射击（用于多人同步）
	 * 非本地控制的客户端执行实际射击逻辑
	 * @param TraceHitTarget - 射击目标位置
	 */
	void LocalShoot(const FVector_NetQuantize& TraceHitTarget);

	/**
	 * 本地霰弹枪射击
	 * @param TraceHitTargets - 多个弹丸目标位置
	 */
	void LocalShotgunShoot(const TArray<FVector_NetQuantize>& TraceHitTargets);

	/**
	 * 服务器RPC - 开火（带验证）
	 * 验证射击间隔防止作弊
	 * @param TraceHitTarget - 目标位置
	 * @param FireDelay - 射击间隔（用于验证）
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget, float FireDelay);

	/**
	 * 多播RPC - 开火
	 * 同步射击效果给所有客户端
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/**
	 * 服务器RPC - 霰弹枪开火（带验证）
	 * @param TraceHitTargets - 多个弹丸目标
	 * @param FireDelay - 射击间隔
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets, float FireDelay);

	/**
	 * 多播RPC - 霰弹枪开火
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	/**
	 * 准心射线检测
	 * 从屏幕中心发射射线检测目标，用于确定射击方向
	 * @param TraceHitResult - 检测结果
	 */
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	/**
	 * 设置HUD准心
	 * 根据移动速度、跳跃状态、瞄准状态计算准心扩散
	 * @param DeltaTime - 帧间隔
	 */
	void SetHUDCrossHairs(float DeltaTime);

	/**
	 * 服务器RPC - 换弹
	 */
	UFUNCTION(Server, Reliable)
	void ServerReload();

	/** 处理换弹动画 */
	void HandleReload();

	/**
	 * 计算需要装填的弹药数量
	 * @return 装填数量
	 */
	int32 AmountToReload();

	/** 投掷手雷 */
	void ThrowGrenade();

	/**
	 * 服务器RPC - 投掷手雷
	 */
	UFUNCTION(Server, Reliable)
	void Server_ThrowGrenade();

	/** 手雷类 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	/** 丢弃当前装备武器 */
	void DropEquippedWeapon();

	/** 附加 actor 到右手 */
	void AttachActorToRightHand(AActor* ActorToAttach);

	/** 附加 actor 到左手 */
	void AttachActorToLeftHand(AActor* ActorToAttach);

	/** 附加 actor 到背包 */
	void AttachActorToBackpack(AActor* ActorToAttach);

	/** 附加旗帜到右手 */
	void AttachFlagToRightHand(AWeaponBase* Flag);

	/** 更新携带弹药显示 */
	void UpdateCarriedAmmo();

	/** 播放装备武器音效 */
	void PlayEquipSound(AWeaponBase* WeaponToEquip);

	/** 空弹匣时自动换弹 */
	void ReloadEmptyWeapon();

	/** 显示/隐藏附加的手雷 */
	void ShowAttachedGrenade(bool bShowGrenade);

	/** 装备主武器 */
	void EquipPrimaryWeapon(AWeaponBase* WeaponToEquip);

	/** 装备副武器 */
	void EquipSecondaryWeapon(AWeaponBase* WeaponToEquip);

private:
	/** 拥有的角色 */
	UPROPERTY()
	ABaseCharacter* Character;

	/** 玩家控制器（缓存） */
	UPROPERTY()
	class ABasePlayerController* Controller;

	/** HUD（缓存） */
	UPROPERTY()
	class APlayerHUD* HUD;

	/** 视口尺寸 */
	FVector2D ViewportSize;

	/** 当前装备的主武器（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeaponBase* EquippedWeapon;

	/** 副武器（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeaponBase* SecondaryWeapon;

	/** 是否正在瞄准（同步） */
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Aiming)
	bool bAiming = false;

	/** 瞄准按键是否按下（本地记录） */
	bool bAimButtonPressed = false;

	/**
	 * RepNotify - 瞄准状态变化
	 * 本地控制的角色需要特殊处理
	 */
	UFUNCTION()
	void OnRep_Aiming();

	/** 基础移动速度 */
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	/** 瞄准时的移动速度 */
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	/** 射击按键是否按下 */
	bool bShootPressed;

	/** 准心扩散因子 - 移动速度 */
	float CrosshairVelocityFactor;

	/** 准心扩散因子 - 空中 */
	float CrosshairInAirFactor;

	/** 准心扩散因子 - 瞄准 */
	float CrosshairAimFactor;

	/** 准心扩散因子 - 射击后坐力 */
	float CrosshairShootingFactor;

	/** 射线检测终点 */
	FVector End;

	/** 射线检测结果 */
	FHitResult HitResult;

	/** 当前射击目标位置 */
	FVector HitTarget;

	/** HUD数据包 */
	FHUDPackage HUDPackage;

	/** 默认FOV */
	float DefaultFOV;

	/** 瞄准时的FOV */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomedFOV = 30.f;

	/** 当前FOV */
	float CurrentFOV;

	/** FOV插值速度 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZoomInterpSpeed = 20.f;

	/** FOV插值 */
	void InterpFOV(float DeltaTime);

	/** 射击计时器句柄 */
	FTimerHandle FireTimer;

	/** 是否可以射击 */
	bool bCanFire = true;

	/** 开始射击计时器（控制射速） */
	void StartFireTimer();

	/** 射击计时结束 */
	void FireTimerFinished();

	/**
	 * 检查是否可以射击
	 * 检查条件：弹药、射击间隔、战斗状态
	 */
	bool CanFire();

	/** 当前携带的弹药数量（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	/**
	 * RepNotify - 携带弹药变化
	 */
	UFUNCTION()
	void OnRep_CarriedAmmo();

	/** 各武器类型的携带弹药映射表 */
	TMap<EWeaponType, int32> CarriedAmmoMap;

	/** 最大携带弹药 */
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	/** 初始突击步枪弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	/** 初始火箭筒弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	/** 初始手枪弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	/** 初始冲锋枪弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	/** 初始霰弹枪弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingShotGunAmmo = 0;

	/** 初始狙击枪弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	/** 初始榴弹发射器弹药 */
	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	/** 初始化携带弹药 */
	void InitializeCarriedAmmo();

	/** 战斗状态（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	/**
	 * RepNotify - 战斗状态变化
	 * 根据状态执行相应操作（换弹动画、投掷手雷等）
	 */
	UFUNCTION()
	void OnRep_CombatState();

	/** 更新弹药数值 */
	void UpdateAmmoValues();

	/** 更新霰弹枪弹药（逐发装填） */
	void UpdateShotgunAmmoValues();

	/** 手雷数量（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 1;

	/**
	 * RepNotify - 手雷数量变化
	 */
	UFUNCTION()
	void OnRep_Grenades();

	/** 最大手雷数量 */
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	/** 更新HUD手雷显示 */
	void UpdateHUDGrenades();

	/** 是否持有旗帜（同步） */
	UPROPERTY(ReplicatedUsing = OnRep_HoldingTheFlag)
	bool bHoldTheFlag = false;

	/**
	 * RepNotify - 持旗状态变化
	 */
	UFUNCTION()
	void OnRep_HoldingTheFlag();

	/** 旗帜武器引用 */
	UPROPERTY()
	AWeaponBase* TheFlag;

public:
	/** 获取手雷数量 */
	FORCEINLINE int32 GetGrenades() const { return Grenades; }

	/** 是否可以切换武器 */
	bool ShouldSwapWeapons();
};
