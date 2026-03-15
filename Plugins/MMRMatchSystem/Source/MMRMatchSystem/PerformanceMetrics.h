#pragma once

/**
 * @file PerformanceMetrics.h
 * @brief 玩家表现数据定义
 * 
 * 定义玩家在对局中的表现指标和事件记录
 */

#include "CoreMinimal.h"
#include "PerformanceMetrics.generated.h"

/**
 * @brief 游戏事件结构
 * 
 * 记录游戏中的关键事件，如：
 * - 连杀
 * - 超神
 * - 一血
 * - 其他影响表现分的事件
 */
USTRUCT(BlueprintType)
struct FGameEvent
{
	GENERATED_BODY()
	
	/** 事件类型标识符 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EventType;
	
	/** 事件数值（如连杀数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;
	
	/** 事件发生时间戳 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Timestamp;
};

/**
 * @brief 表现指标结构
 * 
 * 存储玩家单局游戏的完整表现数据，
 * 用于MMR算法中的个人表现评分
 */
USTRUCT(BlueprintType)
struct FPerformanceMetrics
{
	GENERATED_BODY()
	
	/** KDA比率（击杀+助攻/死亡） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KDA = 0.f;
	
	/** 造成伤害总量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageDone = 0.f;
	
	/** 治疗总量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealingDone = 0.f;
	
	/** 目标贡献分（占点、夺旗等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObjectiveScore = 0.f;
	
	/** 存活时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SurvivalTime = 0.0f;
	
	/**
	 * @brief 计算综合表现分
	 * @return 表现分（0-1范围）
	 * 
	 * 综合所有指标计算加权得分
	 */
	float GetOverallPerformance() const
	{
		float Score = (KDA + DamageDone + HealingDone + ObjectiveScore + SurvivalTime) * 0.3f;
		return FMath::Clamp(Score, 0.f, 1.f);
	}
};
