// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PlayerGameMode.generated.h"

class ABasePlayerState;

/**
 * @brief 扩展的匹配状态命名空间
 * 
 * 在原有UE匹配状态基础上添加冷却状态，用于对局结束后的等待时间
 */
namespace MatchState
{
	/** 对局结束冷却等待下一局开始 */
	extern BATTLEFIELD_API const FName Cooldown;
}

class ABasePlayerController;

/**
 * @brief 基础玩家游戏模式 (PlayerGameMode)
 * 
 * 本游戏的核心游戏模式基类，提供以下功能：
 * - 匹配状态管理（热身->进行->冷却）
 * - 倒计时系统
 * - 玩家淘汰与重生
 * - 分数排行榜追踪
 * - 队伍模式标识
 * 
 * 其他具体游戏模式（团队模式、夺旗模式）继承自此基类
 */
UCLASS()
class BATTLEFIELD_API APlayerGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	/** 构造函数，设置延迟开始 */
	APlayerGameMode();

	/**
	 * @brief 每帧更新
	 * @param DeltaTime 距离上一帧的时间间隔
	 * 
	 * 处理各阶段的倒计时逻辑
	 */
	virtual void Tick(float DeltaTime);

	/**
	 * @brief 处理玩家被淘汰事件
	 * @param EliminatedCharacter 被淘汰的角色
	 * @param VictimController 受害者控制器
	 * @param AttackerController 攻击者控制器
	 * 
	 * 更新分数、排行榜、广播击杀信息
	 */
	virtual void PlayerEliminated(class ABaseCharacter* EliminatedCharacter, 
	                              ABasePlayerController* VictimController, 
	                              ABasePlayerController* AttackerController);

	/**
	 * @brief 请求重生玩家
	 * @param ElimmedCharacter 被消灭的角色（将被销毁）
	 * @param ElimmedController 被消灭的控制器（将在随机出生点重生）
	 */
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	/**
	 * @brief 处理玩家离开游戏
	 * @param PlayerLeaving 正在离开的玩家状态
	 * 
	 * 从排行榜移除并消灭该角色
	 */
	void PlayerLeftGame(ABasePlayerState* PlayerLeaving);

	/**
	 * @brief 计算实际伤害值
	 * @param Attacker 攻击者控制器
	 * @param Victim 受害者控制器
	 * @param BaseDamage 基础伤害值
	 * @return 实际应用的伤害值
	 * 
	 * 基础实现直接返回BaseDamage，子类可重写实现友军伤害等逻辑
	 */
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	//============================
	// 可配置的游戏时间参数
	//============================
	
	/** 热身等待时间（秒），玩家可以在此期间加入 */
	UPROPERTY(EditDefaultsOnly, Category="Game Mode")
	float WarmupTime = 10.f;

	/** 正式匹配时间（秒） */
	UPROPERTY(EditDefaultsOnly, Category="Game Mode")
	float MatchTime = 180.f;

	/** 冷却等待时间（秒），对局结束后等待下一局 */
	UPROPERTY(EditDefaultsOnly, Category="Game Mode")
	float CooldownTime = 10.f;

	/** 关卡开始时间（秒），用于计算倒计时 */
	float LevelStartingTime = 0.f;

	/** 是否为团队模式（影响UI显示等） */
	bool bTeamMatch = false;

protected:
	/** 游戏开始时初始化 */
	virtual void BeginPlay() override;

	/**
	 * @brief 匹配状态改变时调用
	 * 	 * 通知所有玩家控制器匹配状态变化
	 */
	virtual void OnMatchStateSet() override;

private:
	/** 当前倒计时剩余时间 */
	float CountdownTime = 0.f;

public:
	/** 获取当前倒计时时间 */
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
