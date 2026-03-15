// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayerController.generated.h"

class ABaseGameState;
class ABasePlayerState;
class UInputAction;
class UInputMappingContext;
class APlayerState;
class APlayerHUD;
class APlayerGameMode;
class UCharacterOverlay;
class UPauseMenu;

/**
 * @brief 高延迟委托
 * 
 * 当检测到高延迟时广播，用于通知其他系统
 * @param bPingTooHigh 是否延迟过高
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * @brief 基础玩家控制器 (BasePlayerController)
 * 
 * 管理玩家的输入、HUD显示、网络同步和游戏状态：
 * - HUD更新：血量、护盾、弹药、分数、倒计时等
 * - 匹配状态管理：热身、进行中、冷却阶段切换
 * - 时间同步：客户端与服务器时间校准
 * - 网络监控：高延迟检测与警告
 * - 击杀公告：接收并显示击杀信息
 * - 团队分数：红蓝队伍分数显示
 * - 暂停菜单：返回主菜单功能
 */
UCLASS()
class BATTLEFIELD_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	//============================
	// HUD设置函数
	//============================

	/**
	 * @brief 设置HUD血量显示
	 * @param Health 当前血量
	 * @param MaxHealth 最大血量
	 */
	void SetHUDHealth(float Health, float MaxHealth);

	/**
	 * @brief 设置HUD护盾显示
	 * @param Shield 当前护盾
	 * @param MaxShield 最大护盾
	 */
	void SetHUDShield(float Shield, float MaxShield);

	/** 设置HUD分数显示 */
	void SetHUDScore(float Score);

	/** 设置HUD死亡次数显示 */
	void SetHUDDefeats(int32 Defeats);

	/** 设置HUD当前武器弹药显示 */
	void SetHUDWeaponAmmo(int32 Ammo);

	/** 设置HUD携带弹药显示 */
	void SetHUDCarriedAmmo(int32 Ammo);

	/** 设置HUD比赛倒计时显示 */
	void SetHUDMatchCountdown(float CountdownTime);

	/** 设置HUD公告倒计时显示（热身/冷却阶段） */
	void SetHUDAnnouncementCountdown(float CountdownTime);

	/** 设置HUD手雷数量显示 */
	void SetHUDGrenades(int32 Grenades);

	/**
	 * @brief 控制角色时初始化
	 * @param InPawn 被控制的角色
	 * 
	 * 设置初始血量显示
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/**
	 * @brief 每帧更新
	 * @param DeltaTime 距离上一帧的时间间隔
	 * 
	 * 更新HUD时间、检查时间同步、初始化检查、检测延迟
	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief 设置需要复制的属性
	 * @param OutLifetimeProps 输出的属性生命周期数组
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//============================
	// 团队分数
	//============================

	/** 隐藏团队分数显示 */
	void HideTeamScores();

	/** 初始化团队分数显示 */
	void InitTeamScores();

	/** 设置红队分数 */
	void SetHUDRedTeamScore(int32 RedScore);

	/** 设置蓝队分数 */
	void SetHUDBlueTeamScore(int32 BlueScore);

	//============================
	// 时间同步
	//============================

	/**
	 * @brief 获取服务器时间
	 * @return 校准后的服务器时间
	 * 
	 * 服务器直接返回世界时间，客户端返回世界时间加时间差
	 */
	virtual float GetServerTime();

	/**
	 * @brief 接收到玩家时调用
	 * 
	 * 本地控制器开始请求服务器时间同步
	 */
	virtual void ReceivedPlayer() override;

	//============================
	// 匹配状态
	//============================

	/**
	 * @brief 匹配状态设置回调
	 * @param State 新的匹配状态
	 * @param bTeamsMatch 是否为团队模式
	 */
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	/**
	 * @brief 处理比赛开始
	 * @param bTeamsMatch 是否为团队模式
	 * 
	 * 显示角色面板，隐藏公告，初始化团队分数
	 */
	void HandleMatchHasStarted(bool bTeamsMatch = false);

	/**
	 * @brief 处理冷却阶段
	 * 
	 * 显示比赛结果，禁用游戏输入
	 */
	void HandleCooldown();

	/** 单程网络延迟（秒） */
	float SingleTripTime = 0.f;

	/** 高延迟委托 */
	FHighPingDelegate HighPingDelegate;

	/**
	 * @brief 广播击杀公告
	 * @param Attacker 攻击者玩家状态
	 * @param Victim 受害者玩家状态
	 */
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 设置HUD时间显示 */
	void SetHUDTime();

	/**
	 * @brief 轮询初始化
	 * 
	 * HUD可能延迟加载，此函数检查并应用缓存的初始化值
	 */
	void PollInit();

	//============================
	// 时间同步RPC
	//============================

	/**
	 * @brief 服务器RPC：请求服务器时间
	 * @param TimeOfClientRequest 客户端发送请求时的时间
	 */
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	/**
	 * @brief 客户端RPC：报告服务器时间
	 * @param TimeOfClientRequest 客户端发送请求时的时间
	 * @param TimeServerReceivedClientRequest 服务器接收请求时的时间
	 * 
	 * 客户端根据往返时间计算单程延迟和时间差
	 */
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	/** 客户端与服务器的时间差 */
	float ClientServerDelta = 0.f;

	/** 时间同步频率（秒） */
	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;

	/** 时间同步运行时间累计 */
	float TimeSyncRunningTime = 0.f;

	/** 检查是否需要时间同步 */
	void CheckTimeSync(float DeltaTime);

	//============================
	// 匹配状态同步
	//============================

	/**
	 * @brief 服务器RPC：检查当前匹配状态
	 * 
	 * 新玩家加入时获取当前游戏状态
	 */
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	/**
	 * @brief 客户端RPC：加入进行中游戏
	 * @param StateOfMatch 当前匹配状态
	 * @param Warmup 热身时间
	 * @param Match 比赛时间
	 * @param Cooldown 冷却时间
	 * @param StartingTime 关卡开始时间
	 * 
	 * 中途加入的玩家接收当前游戏状态
	 */
	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

	//============================
	// 高延迟警告
	//============================

	/** 显示高延迟警告动画 */
	void HighPingWarning();

	/** 停止高延迟警告动画 */
	void StopHighPingWarning();

	/** 检查网络延迟 */
	void CheckPing(float DeltaTime);

	/** 显示/隐藏暂停菜单 */
	void ShowReturnToMainMenu();

	/**
	 * @brief 客户端RPC：显示击杀公告
	 * @param Attacker 攻击者玩家状态
	 * @param Victim 受害者玩家状态
	 * 
	 * 根据玩家关系显示不同的击杀信息（如"You Killed XXX"）
	 */
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	/**
	 * @brief 是否显示团队分数（网络复制）
	 * 
	 * 变化时触发OnRep_ShowTeamScores
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ShowTeamScores)
	bool bShowTeamScores = false;

	/** 显示团队分数复制回调 */
	UFUNCTION()
	void OnRep_ShowTeamScores();

	/**
	 * @brief 获取混战模式结束信息文本
	 * @param Players 顶部得分玩家列表
	 * @return 格式化后的获胜信息
	 */
	FString GetInfoText(const TArray<ABasePlayerState*>& Players);

	/**
	 * @brief 获取团队模式结束信息文本
	 * @param BaseGameState 游戏状态
	 * @return 格式化后的获胜信息
	 */
	FString GetTeamsInfoText(ABaseGameState* BaseGameState);

protected:
	//============================
	// 输入配置
	//============================

	/** 输入映射上下文 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* ControllerMappingContext;

	/** 退出/暂停操作输入 */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Quit;

private:
	//============================
	// HUD引用
	//============================

	/** 玩家HUD引用 */
	UPROPERTY()
	APlayerHUD* PlayerHUD;

	/** 游戏模式引用 */
	UPROPERTY()
	APlayerGameMode* PlayerGameMode;

	/** 暂停菜单Widget类 */
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> PauseMenuWidget;

	/** 暂停菜单实例 */
	UPROPERTY()
	UPauseMenu* PauseMenu;

	/** 暂停菜单是否打开 */
	bool bPauseMenuOpen = false;

	//============================
	// 匹配时间数据
	//============================

	/** 关卡开始时间 */
	float LevelStartTime = 0.f;

	/** 比赛时间 */
	float MatchTime = 0.f;

	/** 热身时间 */
	float WarmupTime = 0.f;

	/** 冷却时间 */
	float CooldownTime = 0.f;

	/** 倒计时整数（用于优化更新频率） */
	uint32 CountdownInt = 0;

	/**
	 * @brief 当前匹配状态（网络复制）
	 * 
	 * 变化时触发OnRep_MatchState
	 */
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	/** 匹配状态复制回调 */
	UFUNCTION()
	void OnRep_MatchState();

	/** 角色状态面板引用 */
	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	//============================
	// HUD初始化缓存
	//============================
	
	// HUD可能延迟加载，缓存初始值在PollInit中应用

	float HUDHealth;
	float HUDMaxHealth;
	bool bInitializeHealth = false;

	float HUDScore;
	bool bInitializeScore = false;

	int32 HUDDefeats;
	bool bInitializeDefeats = false;

	int32 HUDGrenades;
	bool bInitializeGrenades = false;

	float HUDShield;
	float HUDMaxShield;
	bool bInitializeShield = false;

	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;

	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false;

	//============================
	// 高延迟检测
	//============================

	/** 高延迟检测运行时间累计 */
	float HighPingRunningTime = 0.f;

	/** 高延迟警告持续时间（秒） */
	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingDuration = 5.f;

	/** 高延迟动画运行时间 */
	float PingAnimationRunningTime = 0.f;

	/** 延迟检测频率（秒） */
	UPROPERTY(EditAnywhere, Category = "Network")
	float CheckPingFrequency = 20.f;

	/**
	 * @brief 服务器RPC：报告延迟状态
	 * @param bHighPing 是否高延迟
	 * 
	 * 客户端检测到高延迟时通知服务器
	 */
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);

	/** 高延迟阈值（毫秒） */
	UPROPERTY(EditAnywhere, Category = "Network")
	float HighPingThreshold = 50.f;
};
