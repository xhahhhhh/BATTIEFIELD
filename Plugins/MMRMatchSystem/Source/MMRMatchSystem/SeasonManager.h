#pragma once

/**
 * @file SeasonManager.h
 * @brief 赛季管理器
 * 
 * 管理游戏赛季的生命周期：
 * - 赛季开始/结束时间追踪
 * - 赛季结束时触发软重置
 * - 新赛季自动开始
 */

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SeasonManager.generated.h"

/** 赛季结束委托声明 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonEnded, int32, SeasonNumber);

/**
 * @brief 赛季数据结构
 */
USTRUCT(BlueprintType)
struct FSeasonData
{
	GENERATED_BODY()
	
	/** 赛季编号 */
	UPROPERTY(BlueprintReadOnly)
	int32 SeasonNumber = 1;
	
	/** 开始日期 */
	UPROPERTY(BlueprintReadOnly)
	FDateTime StartDate;
	
	/** 结束日期 */
	UPROPERTY(BlueprintReadOnly)
	FDateTime EndDate;
	
	/** 是否进行中 */
	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;
};

/**
 * @brief 赛季管理器类
 * 
 * 负责赛季的创建、更新和重置
 */
UCLASS(BlueprintType, Blueprintable)
class MMRMATCHSYSTEM_API USeasonManager : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * @brief 初始化赛季
	 * @param Season 赛季编号
	 * @param Start 开始日期
	 * @param End 结束日期
	 */
	void Initialize(int32 Season, const FDateTime& Start, const FDateTime& End);
	
	/**
	 * @brief 更新赛季状态
	 * 
	 * 检查当前时间是否超过结束日期，
	 * 如果是则触发赛季结束并创建新赛季
	 */
	UFUNCTION(BlueprintCallable)
	void UpdateSeason();
	
	/**
	 * @brief 应用软重置
	 * @param CurrentRating 当前评分
	 * @param GamesPlayed 对局数
	 * @param WinRate 胜率
	 * @return 重置后的新评分
	 * 
	 * 新赛季开始时将玩家评分向默认值靠拢，
	 * 但保留部分原有评分以反映实力
	 */
	UFUNCTION(BlueprintCallable)
	float ApplySoftReset(float CurrentRating, int32 GamesPlayed, float WinRate) const;
	
	/** 获取当前赛季数据 */
	UFUNCTION(BlueprintPure)
	FSeasonData GetSeasonData() const { return CurrentSeason; }
	
	/** 赛季结束事件委托 */
	UPROPERTY(BlueprintAssignable)
	FOnSeasonEnded OnSeasonEnded;

protected:
	/** 赛季持续时间（天） */
	UPROPERTY(BlueprintReadOnly, Category="Config")
	int32 SeasonDurationDays = 90;
	
	/** 软重置强度（0-1，越高重置幅度越大） */
	UPROPERTY(BlueprintReadOnly, Category="Config")
	float SoftResetStrength = 0.3f;
	
	/** 默认评分 */
	UPROPERTY(BlueprintReadOnly, Category="Config")
	float DefaultRating = 1500.f;

private:
	/** 当前赛季数据 */
	FSeasonData CurrentSeason;
};
