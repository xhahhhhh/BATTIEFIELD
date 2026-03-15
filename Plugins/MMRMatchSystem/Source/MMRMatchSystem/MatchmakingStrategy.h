#pragma once

/**
 * @file MatchmakingStrategy.h
 * @brief 匹配策略接口和默认实现
 * 
 * 定义匹配策略接口，提供可扩展的匹配算法机制。
 * 默认实现使用贪心算法，优先匹配等待时间长的玩家。
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MatchmakingStrategy.generated.h"

/**
 * @brief 匹配偏好结构
 * 
 * 存储玩家的匹配偏好设置
 */
USTRUCT(BlueprintType)
struct FMatchmakingPreferences
{
	GENERATED_BODY()

	/** 偏好角色列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> PreferredRoles;
};

/**
 * @brief 匹配玩家数据结构
 * 
 * 存储玩家在匹配队列中的完整信息
 */
USTRUCT(BlueprintType)
struct FMatchmakingPlayerData
{
	GENERATED_BODY()
	
	/** 玩家控制器 */
	UPROPERTY()
	APlayerController* Player = nullptr;
	
	/** 匹配评分 */
	UPROPERTY()
	float Rating = 1500.f;
	
	/** 评分不确定性 */
	UPROPERTY()
	float Uncertainty = 400.f;
	
	/** 已等待时间 */
	UPROPERTY()
	float WaitTime = 0.f;
	
	/** 偏好角色 */
	UPROPERTY()
	TArray<FString> PreferredRoles;
	
	/** 连胜数 */
	UPROPERTY()
	int32 WinStreak = 0;
	
	/** 相等运算符（用于TArray操作） */
	bool operator==(const FMatchmakingPlayerData& Other) const
	{
		return Player == Other.Player;
	}
};

/**
 * @brief 匹配结果结构
 * 
 * 存储一场匹配的两队玩家和平衡分数
 */
USTRUCT(BlueprintType)
struct FMatchResult
{
	GENERATED_BODY()
	
	/** A队玩家列表 */
	UPROPERTY()
	TArray<APlayerController*> TeamA;
	
	/** B队玩家列表 */
	UPROPERTY()
	TArray<APlayerController*> TeamB;
	
	/** 平衡分数（越高表示越平衡） */
	UPROPERTY()
	float BalanceScore = 0.f;
};

/** 匹配策略接口声明 */
UINTERFACE()
class UMatchmakingStrategy : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief 匹配策略接口
 * 
 * 蓝图可实现的接口，用于自定义匹配逻辑
 */
class MMRMATCHSYSTEM_API IMatchmakingStrategy
{
	GENERATED_BODY()
	
public:
	/**
	 * @brief 寻找匹配
	 * @param Queue 当前匹配队列
	 * @param TeamSize 每队需要的玩家数
	 * @return 匹配结果数组
	 */
	UFUNCTION(BlueprintNativeEvent)
	TArray<FMatchResult> FindMatches(const TMap<APlayerController*, FMatchmakingPlayerData>& Queue, int32 TeamSize);
	
	/**
	 * @brief 评估匹配质量
	 * @param Match 匹配结果
	 * @return 质量分数（越高越好）
	 */
	UFUNCTION(BlueprintNativeEvent)
	float EvaluateMatchQuality(const FMatchResult& Match) const;
};

/**
 * @brief 默认匹配策略实现
 * 
 * 使用贪心算法：
 * 1. 按等待时间降序排序
 * 2. 以等待最久的玩家为锚点组建队伍
 * 3. 组建对手队伍
 */
UCLASS(BlueprintType)
class UDefaultMatchmakingStrategy : public UObject, public IMatchmakingStrategy
{
	GENERATED_BODY()

public:
	/** 实现匹配查找 */
	virtual TArray<FMatchResult> FindMatches_Implementation(
		const TMap<APlayerController*, FMatchmakingPlayerData>& Queue, 
		int32 TeamSize) override;
	
	/** 实现匹配质量评估 */
	virtual float EvaluateMatchQuality_Implementation(const FMatchResult& Match) const override;
	
	/** 目标平衡阈值（0-1） */
	UPROPERTY(EditDefaultsOnly, Category="Config")
	float TargetBalanceThreshold = 0.7f;
	
	/** 妥协前最大等待时间（秒） */
	UPROPERTY(EditDefaultsOnly, Category="Config")
	float MaxWaitTimeBeforeCompromise = 1800.0f;
	
	/** 是否启用角色匹配 */
	UPROPERTY(EditDefaultsOnly, Category="Config")
	bool bEnableRoleMatching = true;
};
