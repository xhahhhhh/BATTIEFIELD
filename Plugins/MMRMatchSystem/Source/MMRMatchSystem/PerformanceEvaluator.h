#pragma once

/**
 * @file PerformanceEvaluator.h
 * @brief 表现评估策略接口
 * 
 * 提供可扩展的玩家表现评估机制，
 * 支持按角色类型使用不同的评估权重
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "PerformanceMetrics.h"
#include "PerformanceEvaluator.generated.h"

/**
 * @brief 角色权重配置结构
 * 
 * 为特定角色类型定义事件权重映射
 */
USTRUCT(BlueprintType)
struct FRoleWeightConfig
{
	GENERATED_BODY()

	/** 事件类型 -> 权重的映射 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, float> EventWeights;
};

/** 表现评估策略接口声明 */
UINTERFACE(BlueprintType)
class UPerformanceEvaluationStrategy : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief 表现评估策略接口
 * 
 * 定义评估玩家表现的方法
 */
class MMRMATCHSYSTEM_API IPerformanceEvaluationStrategy
{
	GENERATED_BODY()

public:
	/**
	 * @brief 评估玩家表现
	 * @param PlayerEvents 玩家在游戏中的事件列表
	 * @param Role 玩家使用的角色类型
	 * @return 表现分（0-1范围）
	 */
	UFUNCTION(BlueprintNativeEvent)
	float EvaluatePerformance(const TArray<FGameEvent>& PlayerEvents, const FString& Role) const;
};

/**
 * @brief 表现评估器实现类
 * 
 * 根据事件和角色权重计算玩家表现分
 */
UCLASS(BlueprintType)
class UPerformanceEvaluator : public UObject, public IPerformanceEvaluationStrategy
{
	GENERATED_BODY()

public:
	/**
	 * @brief 实现表现评估
	 * @param PlayerEvents 游戏事件列表
	 * @param Role 角色类型
	 * @return 归一化的表现分（0-1）
	 */
	virtual float EvaluatePerformance_Implementation(const TArray<FGameEvent>& PlayerEvents, const FString& Role) const override;
	
	/** 通用事件权重配置 */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	FRoleWeightConfig TotalEventWeights;
	
	/** 角色特定权重配置（角色名 -> 权重配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TMap<FString, FRoleWeightConfig> RoleSpecificWeights;
};
