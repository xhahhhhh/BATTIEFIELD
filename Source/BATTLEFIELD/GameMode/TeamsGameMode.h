// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PlayerGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * @brief 团队游戏模式 (TeamsGameMode)
 * 
 * 继承自PlayerGameMode，添加团队相关功能：
 * - 自动队伍分配（红队/蓝队）
 * - 队伍伤害免疫（队友之间无法造成伤害）
 * - 玩家离开时的队伍清理
 * - 团队计分
 * 
 * 适用于团队死斗等团队对抗玩法
 */
UCLASS()
class BATTLEFIELD_API ATeamsGameMode : public APlayerGameMode
{
	GENERATED_BODY()

public:
	/** 构造函数，标记为团队模式 */
	ATeamsGameMode();

	/**
	 * @brief 玩家登录后的处理
	 * @param NewPlayer 新登录的玩家控制器
	 * 
	 * 为新玩家自动分配到人数较少的队伍
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/**
	 * @brief 玩家退出时的处理
	 * @param Exiting 正在退出的控制器
	 * 
	 * 从队伍中移除退出玩家
	 */
	virtual void Logout(AController* Exiting) override;

	/**
	 * @brief 计算实际伤害值（考虑友军免疫）
	 * @param Attacker 攻击者控制器
	 * @param Victim 受害者控制器
	 * @param BaseDamage 基础伤害值
	 * @return 实际应用的伤害值（队友为0）
	 */
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;

	/**
	 * @brief 处理玩家被淘汰事件
	 * @param EliminatedCharacter 被淘汰的角色
	 * @param VictimController 受害者控制器
	 * @param AttackerController 攻击者控制器
	 * 
	 * 为攻击者所属队伍增加分数
	 */
	virtual void PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
	                              ABasePlayerController* VictimController,
	                              ABasePlayerController* AttackerController) override;

protected:
	/**
	 * @brief 匹配开始时的处理
	 * 
	 * 为尚未分配队伍的玩家自动分配队伍
	 */
	virtual void HandleMatchHasStarted() override;
};
