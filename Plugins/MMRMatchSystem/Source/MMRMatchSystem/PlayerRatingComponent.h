#pragma once

/**
 * @file PlayerRatingComponent.h
 * @brief 玩家评分组件
 * 
 * 该组件附加到PlayerController上，管理玩家的评分数据：
 * - 隐藏分（Hidden Rating）计算
 * - 不确定性（Uncertainty）管理
 * - 连胜/连败追踪
 * - 角色熟练度
 * - 支持ELO和MMR两种算法
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RatingAlgorithm.h"
#include "PlayerRatingComponent.generated.h"

struct FPerformanceMetrics;

/**
 * @brief 评分算法类型枚举
 */
UENUM(BlueprintType)
enum class ERatingAlgorithmType : uint8
{
	ELO		UMETA(DisplayName = "ELO"),		// 经典ELO算法
	MMR		UMETA(DisplayName = "MMR"),		// 改进的MMR算法（含表现分）
	MAX		UMETA(DisplayName = "DefaultMax")
};

/**
 * @brief 玩家评分数据结构
 * 
 * 保存玩家的所有评分相关数据，支持存档（SaveGame标记）
 */
USTRUCT(BlueprintType)
struct FPlayerRatingData
{
	GENERATED_BODY()
	
	/** 隐藏分 - 实际用于匹配计算 */
	UPROPERTY(SaveGame)
	float HiddenRating = 1500.f;
	
	/** 不确定性 - 新玩家不确定性高，随对局减少 */
	UPROPERTY(SaveGame)
	float Uncertainty = 400.f;
	
	/** 总对局数 */
	UPROPERTY(SaveGame)
	int32 GamesPlayed = 0;
	
	/** 胜利数 */
	UPROPERTY(SaveGame)
	int32 Wins = 0;
	
	/** 最近表现分数记录（用于趋势分析） */
	UPROPERTY(SaveGame)
	TArray<float> RecentPerformances;
	
	/** 角色熟练度映射（角色名 -> 熟练度0-1） */
	UPROPERTY(SaveGame)
	TMap<FString, float> RoleProficiency;
	
	/** 评分历史记录（最多50场） */
	UPROPERTY(SaveGame)
	TArray<float> RatingHistory;
	
	/** 当前连胜数 */
	UPROPERTY(SaveGame)
	int32 WinStreak = 0;
	
	/** 当前连败数 */
	UPROPERTY(SaveGame)
	int32 LoseStreak = 0;
	
	/** 最后更新时间 */
	UPROPERTY(SaveGame)
	FDateTime LastUpdateTime;
};

/**
 * @brief 玩家评分组件类
 * 
 * 管理单个玩家的评分系统，支持：
 * - 团队游戏评分更新
 * - 单挑评分更新
 * - 可见段位获取
 * - 连胜追踪
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MMRMATCHSYSTEM_API UPlayerRatingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	UPlayerRatingComponent();

	/** 根据配置初始化算法实例 */
	void ConfigAlgorithmInstance();

	/** 组件开始播放时调用，初始化算法 */
	virtual void BeginPlay() override;

	/** 
	 * @brief 切换评分算法类型
	 * @param NewAlgorithmType 新的算法类型（ELO或MMR）
	 */
	void SetRatingAlgorithm(ERatingAlgorithmType NewAlgorithmType);

	/**
	 * @brief 更新团队游戏后的评分
	 * @param TeamAverageRating 己方团队平均评分
	 * @param OpponentAverageRating 对方团队平均评分
	 * @param bWon 是否获胜
	 * @param Performance 个人表现数据
	 * @param Role 使用的角色（用于更新角色熟练度）
	 */
	void UpdateRatingForTeamGame(
		float TeamAverageRating,
		float OpponentAverageRating,
		bool bWon,
		const FPerformanceMetrics& Performance,
		const FString& Role = TEXT("")
	);

	/**
	 * @brief 更新单挑后的评分
	 * @param OpponentRating 对手评分
	 * @param bWon 是否获胜
	 * @param KFactor K因子（-1使用默认值）
	 */
	void UpdateRatingForDuel(float OpponentRating, bool bWon, float KFactor = -1.0f);

	/** 获取用于匹配的评分（考虑不确定性浮动） */
	float GetMatchmakingRating() const;

	/** 获取可见段位名称 */
	FString GetVisibleRank() const;

	/** 获取段位颜色 */
	FLinearColor GetRankColor() const;

protected:
	/** 当前使用的算法类型 */
	UPROPERTY(BlueprintReadOnly, Category="Rating")
	ERatingAlgorithmType AlgorithmType = ERatingAlgorithmType::MMR;
	
	/** 评分配置数据资产 */
	UPROPERTY(BlueprintReadOnly, Category="Rating")
	class URatingConfigDataAsset* RatingConfig;
	
	/** 玩家评分数据（存档支持） */
	UPROPERTY(BlueprintReadOnly, Category="Rating", SaveGame)
	FPlayerRatingData RatingData;

private:
	/** 算法实例（智能指针自动管理内存） */
	TUniquePtr<IRatingAlgorithm> AlgorithmInstance;
	
	/** 更新连胜/连败计数 */
	void UpdateStreak(bool bWon);

public:
	/** 获取当前不确定性 */
	float GetUncertainty() const { return RatingData.Uncertainty; }
	
	/** 获取当前连胜数 */
	int32 GetWinStreak() const { return RatingData.WinStreak; }
};
