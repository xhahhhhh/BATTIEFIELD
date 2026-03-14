#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../CharacterTypes/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "../CharacterTypes/CombatState.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "BaseCharacter.generated.h"

class APlayerGameMode;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame); // 玩家离开游戏时广播的委托

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UInputMappingContext;
class UWidgetComponent;
class AWeaponBase;
class UCombatComponent;
class UAnimMontage;
class AController;
class UBuffComponent;
class UBoxComponent;

/**
 * 游戏基础角色类
 * 
 * 这是一个多人射击游戏的核心角色类，支持：
 * - 第三人称视角和相机系统
 * - 增强输入系统（Enhanced Input）
 * - 武器系统（装备、切换、射击、换弹）
 * - 生命值与护盾系统
 * - 网络同步与延迟补偿
 * - 死亡淘汰与重生机制
 * - 团队系统
 * 
 * 网络架构：
 * - 服务器权威性游戏逻辑
 * - 客户端预测与服务器验证
 * - 延迟补偿用于命中检测
 */
UCLASS()
class BATTLEFIELD_API ABaseCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	/** @name 构造函数与基类重载 */
	///@{
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	///@}
	
	/** @name 动画蒙太奇播放方法 */
	///@{
	/** 
	 * 播放射击动画
	 * @param bAiming 是否处于瞄准状态，决定使用哪一段动画
	 */
	void PlayFireMontage(bool bAiming);
	
	/** 播放淘汰（死亡）动画 */
	void PlayElimMontage();
	
	/** 
	 * 播放换弹动画
	 * 根据当前装备的武器类型自动选择对应的动画段落
	 */
	void PlayReloadMontage();
	
	/** 播放投掷手雷动画 */
	void PlayThrowGrenadeMontage();
	
	/** 播放切换武器动画 */
	void PlaySwapMontage();
	///@}
	
	/** @name 网络同步与游戏状态 */
	///@{
	/** 
	 * 网络移动同步回调
	 * 当服务器同步移动数据到客户端时调用，用于处理模拟代理的转身动画
	 */
	virtual void OnRep_ReplicatedMovement() override;
	
	/** 
	 * 淘汰（杀死）角色
	 * @param bPlayerLeftGame 是否因为玩家离开游戏而淘汰
	 */
	void Elim(bool bPlayerLeftGame);

	/** 
	 * 多播淘汰效果
	 * 在所有客户端上执行淘汰相关的视觉效果和状态设置
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	/** 角色销毁时的清理逻辑 */
	virtual void Destroyed() override;

	/** 禁用游戏玩法的标志（如淘汰后） */
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	///@}
	
	/** @name HUD与UI */
	///@{
	/** 
	 * 蓝图实现事件：显示/隐藏狙击镜UI
	 * @param bShowScope true显示，false隐藏
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	/** 更新HUD上的生命值显示 */
	void UpdateHUDHealth();
	
	/** 更新HUD上的护盾值显示 */
	void UpdateHUDShield();
	
	/** 更新HUD上的弹药显示 */
	void UpdateHUDAmmo();
	///@}
	
	/** 生成默认武器 */
	void SpawnDefaultWeapon();

	/** 
	 * 身体部位命中检测碰撞盒映射
	 * 用于服务器端延迟补偿的射线检测
	 */
	UPROPERTY()
	TMap<FName,UBoxComponent*> HitCollisionBoxes;
	
	/** 切换武器动画是否完成的标志 */
	bool bFinishedSwapping = false;
	
	/** @name 网络RPC方法（服务器调用） */
	///@{
	/** 客户端调用：通知服务器玩家离开游戏 */
	UFUNCTION(Server,Reliable)
	void ServerLeaveGame();
	
	/** 玩家离开游戏时广播的委托实例 */
	FOnLeftGame OnLeftGame;
	///@}
	
	/** @name 领先玩家特效 */
	///@{
	/** 多播：获得领先位置时在所有客户端播放皇冠特效 */
	UFUNCTION(NetMulticast,Reliable)
	void MulticastGainedTheLead();
	
	/** 多播：失去领先位置时销毁皇冠特效 */
	UFUNCTION(NetMulticast,Reliable)
	void MulticastLostTheLead();
	///@}
	
	/** 
	 * 根据团队设置角色材质颜色
	 * @param Team 所属团队
	 */
	void SetTeamColor(ETeam Team);
	
protected:
	/** @name 增强输入系统配置 */
	///@{
	/** 输入映射上下文 - 包含所有输入操作与按键的映射关系 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* MappingContext;

	/** 移动输入动作（WASD/摇杆） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Move;

	/** 视角查看输入动作（鼠标/右摇杆） */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Look;

	/** 跳跃输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Jump;

	/** 装备武器输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Equip;

	/** 蹲下输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Crouch;

	/** 瞄准输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Aim;

	/** 射击输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Shoot;

	/** 换弹输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Reload;

	/** 投掷手雷输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Throw;
	
	/** 切换武器输入动作 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_SwapWeapon;
	///@}

protected:
	/** 游戏开始时初始化 */
	virtual void BeginPlay() override;

	/** @name 输入动作回调函数 */
	///@{
	/**
	 * 移动回调
	 * 根据输入向量计算前后左右移动方向
	 */
	void Move(const FInputActionValue& Value);
	
	/**
	 * 视角查看回调
	 * 根据输入向量旋转控制器视角
	 */
	void Look(const FInputActionValue& Value);
	
	/** 跳跃回调（处理蹲下状态） */
	void DoJump();
	
	/** 装备按钮按下回调 - 尝试装备重叠的武器 */
	void EquipButtonPressed();
	
	/** 蹲下按钮按下回调 - 切换蹲下/站立状态 */
	void DoCrouch();
	
	/** 换弹按钮按下回调 */
	void ReloadPressed();
	
	/** 瞄准开始回调 */
	void Aim();
	
	/** 瞄准结束回调 */
	void AimEnd();
	
	/** 重载跳跃逻辑，处理蹲下状态 */
	virtual void Jump() override;
	
	/** 射击按钮按下回调 */
	void ShootPressed();
	
	/** 射击按钮释放回调 */
	void ShootReleased();
	
	/** 投掷按钮按下回调 */
	void ThrowPressed();
	
	/** 切换武器按钮按下回调 */
	void SwapWeapon();
	///@}

	/** @name 瞄准偏移与转身系统 */
	///@{
	/**
	 * 计算瞄准偏移（Aim Offset）
	 * 用于混合空间动画，实现角色上半身跟随准星
	 */
	void AimOffset(float DeltaTime);
	
	/**
	 * 计算俯仰角（Pitch）
	 * 处理俯仰角的范围转换（本地客户端与服务器的差异）
	 */
	void CalculateAO_Pitch();
	
	/**
	 * 模拟代理转身处理
	 * 用于其他玩家角色在网络同步时的转身动画
	 */
	void SimProxiesTurn();
	///@}

	/**
	 * 播放受击反应动画
	 * 在角色受到伤害时播放
	 */
	void PlayHitReactMontage();

	/** 
	 * 丢弃或销毁单个武器
	 * @param Weapon 要处理的武器
	 */
	void DropOrDestroyWeapon(AWeaponBase* Weapon);

	/** 丢弃或销毁所有武器（淘汰时调用） */
	void DropOrDestroyWeapons();
	
	/** 
	 * 设置重生点
	 * 根据玩家所属团队在对应团队出生点中随机选择
	 */
	void SetSpawnPoint();
	
	/** 玩家状态初始化完成时的回调 */
	void OnPlayerStateInitialized();
	
	/**
	 * 接收伤害回调
	 * 绑定到OnTakeAnyDamage委托，处理所有伤害计算
	 */
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	                   AController* InstigatorController, AActor* DamageCauser);
	
	/** 
	 * 轮询初始化
	 * 等待PlayerState可用后初始化HUD
	 */
	void PollInit();
	
	/**
	 * 在原地旋转角色
	 * 处理瞄准偏移和转身动画的逻辑
	 */
	void RotateInPlace(float DeltaTime);

	/** @name 服务器倒带命中检测碰撞盒（身体各部位） */
	///@{
	/** 头部命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* head;
	
	/** 骨盆命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	
	/** 脊柱第一节命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_01;
	
	/** 脊柱第二节命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	
	/** 脊柱第三节命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;
	
	/** 脊柱第四节命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_04;
	
	/** 脊柱第五节命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_05;
	
	/** 左大臂命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;
	
	/** 右大臂命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;
	
	/** 左小臂命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;
	
	/** 右小臂命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;
	
	/** 左手命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;
	
	/** 右手命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;
	
	/** 左大腿命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	
	/** 右大腿命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;
	
	/** 左小腿命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;
	
	/** 右小腿命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;
	
	/** 左脚命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;
	
	/** 右脚命中盒 */
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;
	///@}

private:
	/** @name 相机系统组件 */
	///@{
	/** 弹簧臂组件 - 控制相机与角色的距离和旋转 */
	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	/** 跟随相机 - 实际渲染视角的相机组件 */
	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;
	///@}

	/** 
	 * 头顶UI组件 - 显示玩家名称、血条等信息
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverHeadWidget;

	/**
	 * 当前重叠的可拾取武器
	 * 当角色靠近可拾取武器时设置
	 */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeaponBase* OverlappingWeapon;

	/** @name 游戏逻辑组件 */
	///@{
	/** 
	 * 战斗组件 - 处理武器、射击、换弹等战斗逻辑
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	/**
	 * 增益组件 - 处理速度、跳跃等增益效果
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UBuffComponent* Buff;

	/**
	 * 延迟补偿组件 - 服务器端处理命中检测的延迟补偿
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class ULagCompensationComponent* LagCompensation;
	///@}
	
	/** @name 网络RPC方法 */
	///@{
	/** 
	 * 服务器RPC：装备武器
	 * 客户端请求服务器执行装备武器逻辑
	 */
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	/**
	 * 服务器RPC：切换武器
	 * 客户端请求服务器执行切换武器逻辑
	 */
	UFUNCTION(Server, Reliable)
	void ServerSwapWeapon();
	///@
	
	/**
	 * 重叠武器变化时的回调
	 * 处理拾取UI的显示/隐藏
	 */
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeaponBase* Lastweapon);

	/** @name 瞄准偏移（Aim Offset）变量 */
	///@{
	/** 偏航角偏移 - 角色身体相对于控制器的水平角度差 */
	float AO_Yaw;
	
	/** 插值后的偏航角偏移 - 用于平滑转身动画 */
	float InterpAO_Yaw;
	
	/** 俯仰角偏移 - 角色身体相对于控制器的垂直角度差 */
	float AO_Pitch;
	
	/** 起始瞄准旋转 - 用于计算角度变化 */
	FRotator StartingAimRotation;
	///@}

	/**
	 * 当前转身状态
	 * 用于控制转身动画的播放
	 */
	ETurningInPlace TurningInPlace;
	
	/**
	 * 转身逻辑处理
	 * 根据AO_Yaw的值决定向左还是向右转身
	 */
	void TurnInPlace(float DeltaTime);

	/** @name 动画蒙太奇资源 */
	///@{
	/** 射击动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireMontage;

	/** 受击反应动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	/** 淘汰（死亡）动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	/** 
	 * 换弹动画蒙太奇
	 * 包含多种武器的换弹段落（Rifle, Pistol, SMG等）
	 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	/** 投掷手雷动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;
	
	/** 切换武器动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage;
	///@

	/**
	 * 当相机距离角色过近时隐藏角色网格
	 * 防止相机穿模影响视觉效果
	 */
	void HideCameraIfCharacterClose();

	/** 相机隐藏阈值距离 */
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	/** @name 转身与网络同步变量 */
	///@{
	/** 是否旋转根骨骼（站立瞄准时为true） */
	bool bRotateRootBone;
	
	/** 转身阈值 - 角度变化超过此值触发转身动画 */
	float TurnThreshold = .5f;
	
	/** 上一帧的代理旋转（用于模拟代理） */
	FRotator ProxyRotationLastFrame;
	
	/** 当前代理旋转 */
	FRotator ProxyRotation;
	
	/** 代理偏航角变化 */
	float ProxyYaw;
	
	/** 距离上次网络移动同步的时间 */
	float TimeSinceLastMovementReplication;
	///@}

	/** 计算当前水平移动速度 */
	float CalculateSpeed();

	/** @name 生命值系统 */
	///@{
	/** 最大生命值 */
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxHealth = 100.f;

	/** 当前生命值（网络同步） */
	UPROPERTY(Replicatedusing = OnRep_Health, VisibleAnywhere, Category = "PlayerStats")
	float Health = 100.f;

	/**
	 * 生命值变化回调
	 * 用于更新HUD和播放受击动画
	 */
	UFUNCTION()
	void OnRep_Health(float LastHealth);
	///@}

	/** @name 护盾系统 */
	///@{
	/** 最大护盾值 */
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxShield = 100.f;

	/** 当前护盾值（网络同步） */
	UPROPERTY(Replicatedusing = OnRep_Shield, EditAnywhere, Category = "PlayerStats")
	float Shield = 0.f;

	/**
	 * 护盾值变化回调
	 * 用于更新HUD
	 */
	UFUNCTION()
	void OnRep_Shield(float LastShield);
	///@

	/**
	 * 玩家控制器缓存
	 * 用于快速访问，避免重复Cast
	 */
	UPROPERTY()
	class ABasePlayerController* BasePlayerController;

	/** 是否已被淘汰 */
	bool bElimmed = false;

	/** 淘汰计时器句柄 */
	FTimerHandle ElimTimer;

	/** 淘汰延迟时间（从死亡到重生） */
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	/** 淘汰计时器完成回调 */
	void ElimTimerFinished();
	
	/** 是否因离开游戏而淘汰 */
	bool bLeftGame = false;
	
	/** @name 溶解效果系统 */
	///@{
	/** 溶解时间轴组件 - 用于材质溶解动画 */
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	/** 时间轴进度委托 */
	FOnTimelineFloat DissolveTrack;

	/** 溶解曲线 - 控制溶解动画的速度曲线 */
	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	/**
	 * 更新溶解材质参数
	 * @param DissolveValue 溶解值（0-1）
	 */
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	/** 开始溶解动画 */
	void StartDissolve();

	/** 动态溶解材质实例 */
	UPROPERTY(VisibleAnywhere, Category=Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	/** 溶解材质实例模板 */
	UPROPERTY(VisibleAnywhere, Category=Elim)
	UMaterialInstance* DissolveMaterialInstance;
	///@}
	
	/** @name 团队标识材质 */
	///@{
	/** 红色团队溶解材质 */
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* RedDissolveMatInst;
	
	/** 红色团队基础材质 */
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* RedMaterial;
	
	/** 蓝色团队溶解材质 */
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* BlueDissolveMatInst;
	
	/** 蓝色团队基础材质 */
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* BlueMaterial;
	
	/** 原始/无团队材质 */
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* OriginalMaterial;
	///@
	/** @name 淘汰特效 */
	///@{
	/** 淘汰粒子效果资源 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimEffect;

	/** 淘汰粒子效果组件实例 */
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimEffectComponent;

	/** 淘汰音效资源 */
	UPROPERTY(EditAnywhere)
	USoundCue* ElimSound;
	///@}

	/**
	 * 玩家状态缓存
	 * 用于访问分数、击杀数、所属团队等信息
	 */
	UPROPERTY()
	class ABasePlayerState* BasePlayerState;
	
	/** @name 领先玩家皇冠特效 */
	///@{
	/** 皇冠粒子系统资源（Niagara） */
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;
	
	/** 皇冠粒子组件实例 */
	UPROPERTY()
	class UNiagaraComponent* CrownComponent;
	///@}

	/**
	 * 手雷网格组件
	 * 附加在角色左手，投掷时隐藏
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;
	
	/**
	 * 默认武器类
	 * 角色生成时自动装备的武器
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	/**
	 * 游戏模式缓存
	 * 用于请求重生、计算伤害等
	 */
	UPROPERTY()
	APlayerGameMode* PlayerGameMode;
public:
	/** @name 公有访问器方法 */
	///@{
	/**
	 * 设置当前重叠的武器
	 * 由武器触发器调用
	 */
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	
	/** 检查是否已装备武器 */
	bool IsWeaponEquipped();
	
	/** 检查是否处于瞄准状态 */
	bool IsAiming();
	
	/** 获取当前偏航角偏移 */
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	
	/** 获取当前俯仰角偏移 */
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	
	/** 获取当前装备的武器 */
	AWeaponBase* GetEquippedWeapon();
	
	/** 获取当前转身状态 */
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	
	/**
	 * 获取当前瞄准目标位置
	 * 用于IK和武器射击方向计算
	 */
	FVector GetHitTarget() const;
	
	/** 获取跟随相机组件 */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	/** 是否应该旋转根骨骼 */
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	
	/** 检查角色是否已被淘汰 */
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	
	/** 获取当前生命值 */
	FORCEINLINE float GetHealth() const { return Health; }
	
	/** 设置生命值 */
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	
	/** 获取最大生命值 */
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	
	/** 获取当前护盾值 */
	FORCEINLINE float GetShield() const { return Shield; }
	
	/** 设置护盾值 */
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	
	/** 获取最大护盾值 */
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	
	/** 获取当前战斗状态 */
	ECombatState GetCombatState() const;
	
	/** 获取战斗组件 */
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	
	/** 获取是否禁用游戏玩法 */
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	
	/** 获取换弹动画蒙太奇 */
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	
	/** 获取手雷网格组件 */
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	
	/** 获取增益组件 */
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	
	/** 检查是否在本地客户端正在换弹 */
	bool IsLocallyReloading();
	
	/** 获取延迟补偿组件 */
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	
	/** 检查是否正在持有旗帜（夺旗模式） */
	bool IsHoldingTheFlag() const;
	
	/** 获取所属团队 */
	ETeam GetTeam();
	
	/**
	 * 设置是否持有旗帜
	 * @param bHolding true为持有，false为不持有
	 */
	void SetHoldingTheFlag(bool bHolding);
	///@}
};


