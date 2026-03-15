#pragma once

/**
 * @file RatingConfigDataAsset.h
 * @brief 评分配置数据资产
 * 
 * 使用UE数据资产系统存储评分算法的可配置参数，
 * 支持在编辑器中调整而无需修改代码
 */

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RatingConfigDataAsset.generated.h"

/**
 * @brief ELO算法配置结构
 */
USTRUCT(BlueprintType)
struct FELOConfig
{
	GENERATED_BODY()
	
	/** 基础K因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseKFactor = 32.f;
	
	/** 新玩家K因子（用于快速定位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NewPlayerKFactor = 40.f;
	
	/** 新玩家判定阈值（对局数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NewPlayerThreshold = 50;
	
	/** 高分段阈值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighLevelThreshold = 1000;
	
	/** 高分段K因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighLevelKFactor = 16.f;
};

/**
 * @brief MMR算法配置结构
 */
USTRUCT(BlueprintType)
struct FMMRConfig
{
	GENERATED_BODY()
	
	/** 基础K因子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseKFactor = 32.f;
	
	/** 初始不确定性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialUncertainty = 400.f;
	
	/** 不确定性衰减率（每次对局乘以该值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UncertaintyDecay = 0.98f;
	
	/** 最小不确定性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinUncertainty = 50.f;
	
	/** 个人表现对评分的影响系数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PerformanceImpact = 0.2f;
};

/**
 * @brief 段位定义结构
 */
USTRUCT(BlueprintType)
struct FRankTier
{
	GENERATED_BODY()
	
	/** 段位名称（如"白银"、"黄金"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;
	
	/** 最低评分 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinRating;
	
	/** 最高评分（不包含） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxRating;
	
	/** 段位颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color;
};

/**
 * @brief 评分配置数据资产
 * 
 * 集中管理所有评分相关配置，可在编辑器中创建和修改
 */
UCLASS(BlueprintType)
class MMRMATCHSYSTEM_API URatingConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** 默认初始评分 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="General")
	float DefaultRating = 1500.f;
	
	/** ELO算法配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ELO")
	FELOConfig ELOConfig;
	
	/** MMR算法配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MMR")
	FMMRConfig MMRConfig;
	
	/** 段位定义列表 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ranks")
	TArray<FRankTier> RankTiers;
	
	/**
	 * @brief 根据评分获取对应段位
	 * @param Rating 玩家评分
	 * @return 匹配的段位信息
	 */
	UFUNCTION(BlueprintPure)
	FRankTier GetTierForRating(float Rating) const;
};
