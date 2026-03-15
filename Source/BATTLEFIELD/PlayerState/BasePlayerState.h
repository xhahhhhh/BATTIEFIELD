// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "GameFramework/PlayerState.h"
#include "BasePlayerState.generated.h"

class ABaseCharacter;
class ABasePlayerController;

/**
 * @brief 基础玩家状态 (BasePlayerState)
 * 
 * 管理玩家的游戏状态信息，包括：
 * - 分数（继承自APlayerState，自动网络复制）
 * - 死亡次数（自定义网络复制）
 * - 队伍归属（红队/蓝队/无队伍）
 * 
 * 所有状态变化都通过回调函数同步更新HUD显示
 */
UCLASS()
class BATTLEFIELD_API ABasePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置需要复制的属性
	 * @param OutLifetimeProps 输出的属性生命周期数组
	 * 
	 * 配置Defeats和Team的网络复制
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief 分数复制回调
	 * 
	 * 当服务器上的Score变化并复制到客户端时调用
	 * 更新HUD上的分数显示
	 */
	virtual void OnRep_Score() override;

	/**
	 * @brief 死亡次数复制回调
	 * 
	 * 当服务器上的Defeats变化并复制到客户端时调用
	 * 更新HUD上的死亡次数显示
	 */
	UFUNCTION()
	virtual void OnRep_Defeats();

	/**
	 * @brief 增加分数
	 * @param ScoreAmount 要增加的分数值
	 * 
	 * 在服务器上执行，自动同步到客户端
	 */
	void AddToScore(float ScoreAmount);

	/**
	 * @brief 增加死亡次数
	 * @param DefeatsAmount 要增加的死亡次数
	 * 
	 * 在服务器上执行，自动同步到客户端
	 */
	void AddToDefeats(int32 DefeatsAmount);

private:
	/** 缓存的角色引用（用于更新HUD） */
	UPROPERTY()
	ABaseCharacter* Character;

	/** 缓存的控制器引用（用于更新HUD） */
	UPROPERTY()
	ABasePlayerController* Controller;

	/**
	 * @brief 死亡次数
	 * 
	 * 网络复制，变化时触发OnRep_Defeats
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	/**
	 * @brief 所属队伍
	 * 	 * 网络复制，变化时触发OnRep_Team
	 * 默认无队伍（ET_NoTeam）
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;

	/**
	 * @brief 队伍复制回调
	 * 
	 * 当服务器上的Team变化并复制到客户端时调用
	 * 更新角色的队伍颜色显示
	 */
	UFUNCTION()
	void OnRep_Team();

public:
	/** 获取当前队伍 */
	FORCEINLINE ETeam GetTeam() const { return Team; }

	/**
	 * @brief 设置所属队伍
	 * @param TeamToSet 目标队伍
	 * 
	 * 设置队伍并立即更新角色颜色（服务器调用）
	 */
	void SetTeam(ETeam TeamToSet);
};
