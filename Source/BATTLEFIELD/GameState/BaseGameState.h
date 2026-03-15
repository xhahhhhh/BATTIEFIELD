// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BaseGameState.generated.h"

class ABasePlayerState;

/**
 * @brief 基础游戏状态 (BaseGameState)
 * 
 * 管理游戏全局状态信息，包括：
 * - 排行榜：追踪得分最高的玩家列表
 * - 团队分数：红队和蓝队的实时分数
 * - 团队成员：红队和蓝队的玩家列表
 * 
 * 所有关键数据都通过网络复制同步到所有客户端
 */
UCLASS()
class BATTLEFIELD_API ABaseGameState : public AGameState
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置需要复制的属性
	 * @param OutLifetimeProps 输出的属性生命周期数组
	 * 
	 * 配置排行榜、红蓝队伍分数的网络复制
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief 更新排行榜
	 * @param ScoringPlayer 获得分数的玩家状态
	 * 
	 * 根据玩家新分数更新排行榜：
	 * - 如果分数等于当前最高分：加入并列第一列表
	 * - 如果分数超过当前最高分：清空列表，该玩家成为新的第一
	 */
	void UpdateTopScore(ABasePlayerState* ScoringPlayer);

	//============================
	// 排行榜数据
	//============================
	
	/** 当前得分最高的玩家列表（支持并列第一） */
	UPROPERTY(Replicated)
	TArray<ABasePlayerState*> TopScoringPlayers;

	//============================
	// 团队数据
	//============================
	
	/** 增加红队分数并更新HUD */
	void RedTeamScores();
	
	/** 增加蓝队分数并更新HUD */
	void BlueTeamScores();

	/** 红队成员列表（服务器管理，不复制到客户端） */
	TArray<ABasePlayerState*> RedTeam;
	
	/** 蓝队成员列表（服务器管理，不复制到客户端） */
	TArray<ABasePlayerState*> BlueTeam;

	/**
	 * @brief 红队分数（自动复制到客户端）
	 * 使用 OnRep_RedTeamScore 回调在客户端更新HUD
	 */
	UPROPERTY(ReplicatedUsing = OnRep_RedTeamScore)
	float RedTeamScore = 0.f;

	/**
	 * @brief 蓝队分数（自动复制到客户端）
	 * 使用 OnRep_BlueTeamScore 回调在客户端更新HUD
	 */
	UPROPERTY(ReplicatedUsing = OnRep_BlueTeamScore)
	float BlueTeamScore = 0.f;

	/**
	 * @brief 红队分数复制回调
	 * 当 RedTeamScore 在客户端更新时自动调用，更新HUD显示
	 */
	UFUNCTION()
	void OnRep_RedTeamScore();

	/**
	 * @brief 蓝队分数复制回调
	 * 当 BlueTeamScore 在客户端更新时自动调用，更新HUD显示
	 */
	UFUNCTION()
	void OnRep_BlueTeamScore();

private:
	/** 当前最高分数（用于排行榜比较） */
	float TopScore = 0.f;
};
