#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "WeaponBase.generated.h"

class USphereComponent;
class UWidgetComponent;
class UTexture2D;

/**
 * @brief 武器状态枚举
 * 
 * 定义武器在游戏中的各种状态
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),           // 初始状态（未拾取）
	EWS_Equipped UMETA(DisplayName = "Equipped State"),         // 已装备（主武器）
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"), // 已装备（副武器）
	EWS_Dropped UMETA(DisplayName = "Dropped State"),           // 丢弃状态

	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

/**
 * @brief 射击类型枚举
 * 
 * 定义武器的射击方式
 */
UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScanWeapon"),       // 即时命中（子弹无飞行时间）
	EFT_Projectile UMETA(DisplayName = "ProjectileWeapon"), // 投射物（子弹有飞行时间）
	EFT_Shotgun UMETA(DisplayName = "ShotgunWeapon"),       // 霰弹枪（多弹丸散射）

	EFT_MAX UMETA(DisplayName = "DefaultMAX")
};

class ACasing;

/**
 * @brief 武器基类 (WeaponBase)
 * 
 * 所有武器的基类，提供通用功能：
 * - 武器状态管理（装备/丢弃/副武器）
 * - 弹药系统（消耗/补充/同步）
 * - 拾取检测和UI
 * - 射击基础功能
 * - 服务器倒带支持
 * - 准星配置
 * 
 * 子类实现具体的射击逻辑（HitScan/Projectile/Shotgun）
 */
UCLASS()
class BATTLEFIELD_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

	/** 设置HUD弹药显示 */
	void SetHUDAmmo();

	/** 显示/隐藏拾取提示Widget */
	void ShowPickupWidget(bool bShowWidget);

	/**
	 * @brief 开火
	 * @param HitTarget 目标位置
	 * 
	 * 播放射击动画、生成弹壳、消耗弹药
	 */
	virtual void Fire(const FVector& HitTarget);

	/** 丢弃武器 */
	virtual void Dropped();

	/** 增加弹药 */
	void AddAmmo(int32 AmmoToAdd);

	/**
	 * @brief 计算带散射的射线终点
	 * @param HitTarget 目标位置
	 * @return 带随机散射的射线终点
	 * 
	 * 在目标位置周围球形范围内随机选择终点
	 */
	FVector TraceEndWithScatter(const FVector& HitTarget);

protected:
	/** 拥有者角色引用 */
	UPROPERTY()
	class ABaseCharacter* OwnerCharacter;

	/** 拥有者控制器引用 */
	UPROPERTY()
	class ABasePlayerController* OwnerController;

	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	/**
	 * @brief 进入拾取范围
	 */
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                             const FHitResult& SweepResult);

	/**
	 * @brief 离开拾取范围
	 */
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//============================
	// 散射配置
	//============================

	/** 散射球体距离枪口的位置 */
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	float DistanceToSphere = 800.f;

	/** 散射球体半径 */
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	float SphereRadius = 75.f;

	/** 基础伤害值 */
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	/** 爆头伤害值 */
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

	/**
	 * @brief 是否使用服务器倒带
	 * 
	 * 高延迟时启用，用于补偿网络延迟造成的命中判定差异
	 */
	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	/**
	 * @brief 延迟过高回调
	 * @param bPingTooHigh 是否延迟过高
	 * 
	 * 延迟过高时禁用服务器倒带
	 */
	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);

private:
	/** 武器网格组件 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	/** 拾取范围球体 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	/**
	 * @brief 武器状态（网络复制）
	 */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	/** 状态复制回调 */
	UFUNCTION()
	void OnRep_WeaponState();

	/** 拾取提示Widget */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickUpWidget;

	/** 射击动画 */
	UPROPERTY(EditAnywhere, Category = "WeaponProperties")
	class UAnimationAsset* FireAnimation;

	/** 弹壳类 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass;

	/** 当前弹药量 */
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	/**
	 * @brief 客户端RPC：更新弹药
	 * @param ServerAmmo 服务器弹药量
	 * 
	 * 服务器通知客户端同步弹药
	 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	/**
	 * @brief 客户端RPC：增加弹药
	 * @param AmmoToAdd 增加的弹药量
	 */
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	/** 消耗一发弹药 */
	void SpendRound();

	/** 弹匣容量 */
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	/**
	 * @brief 待处理请求计数器
	 * 
	 * 用于处理客户端预测和服务器确认的差值
	 * 增加弹药 + 减少弹药 -
	 */
	int32 Sequence = 0;

	/** 武器类型 */
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:
	//============================
	// 准星配置
	//============================

	/** 中心准星 */
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsCenter;

	/** 左准星 */
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsLeft;

	/** 右准星 */
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsRight;

	/** 上准星 */
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsTop;

	/** 下准星 */
	UPROPERTY(EditAnywhere, Category = CrossHairs)
	UTexture2D* CrosshairsButtom;

	//============================
	// 瞄准配置
	//============================

	/** 瞄准FOV */
	UPROPERTY(EditAnywhere)
	float ZoomenFOV = 30.f;

	/** 瞄准插值速度 */
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	//============================
	// 射击配置
	//============================

	/** 射击间隔（秒） */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireDelay = .15f;

	/** 是否全自动 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutomatic = true;

	/** 装备音效 */
	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	/**
	 * @brief 设置自定义深度
	 * @param bEnable 是否启用
	 * 
	 * 用于丢弃时的高亮轮廓效果
	 */
	void EnableCustomDepth(bool bEnable);

	/** 是否标记为销毁 */
	bool bDestroyWeapon = false;

	/** 射击类型 */
	UPROPERTY(EditAnywhere)
	EFireType FireType;

	/** 所属队伍 */
	UPROPERTY(EditAnywhere)
	ETeam Team;

	/** 是否使用散射 */
	UPROPERTY(EditAnywhere, Category = "WeaponScatter")
	bool bUseScatter = false;

public:
	/** 设置武器状态 */
	void SetWeaponState(EWeaponState State);

	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE UWidgetComponent* GetPickUpWidget() const { return PickUpWidget; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomenFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }

	/** 检查弹药是否为空 */
	bool IsEmpty();

	/** 检查弹药是否满 */
	bool IsFull();

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE ETeam GetTeam() const { return Team; }
};
