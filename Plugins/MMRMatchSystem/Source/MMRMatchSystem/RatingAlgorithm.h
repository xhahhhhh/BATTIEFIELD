#pragma once

/**
 * @file RatingAlgorithm.h
 * @brief 评分算法接口和实现
 * 
 * 提供评分算法的抽象接口和两种具体实现：
 * - FELOAlgorithm: 经典ELO算法
 * - FMMRAlgorithm: 改进的MMR算法（含个人表现分）
 */

#include "CoreMinimal.h"
#include "PerformanceMetrics.h"

/**
 * @brief 评分算法抽象接口
 * 
 * 定义所有评分算法必须实现的方法
 */
class IRatingAlgorithm
{
public:
	virtual ~IRatingAlgorithm() = default;

	/**
	 * @brief 计算预期胜率
	 * @param RatingA 玩家A评分
	 * @param RatingB 玩家B评分
	 * @return 预期胜率（0-1）
	 */
	virtual float CalculateExpectedScore(float RatingA, float RatingB) const = 0;

	/**
	 * @brief 更新评分（基础版本）
	 * @param CurrentRating 当前评分
	 * @param OpponentRating 对手评分
	 * @param bWon 是否获胜
	 * @param KFactor K因子（-1使用动态计算）
	 * @return 新评分
	 */
	virtual float UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor = -1.f) const = 0;

	/**
	 * @brief 更新评分（带个人表现）
	 * @param CurrentRating 当前评分
	 * @param TeamAverageRating 己方团队平均评分
	 * @param OpponentAvarageRating 对方团队平均评分
	 * @param bWon 是否获胜
	 * @param Performance 个人表现数据
	 * @param Uncertainty 当前不确定性
	 * @param GamesPlayed 已进行对局数
	 * @param OutUncertaintyDelta 输出不确定性变化
	 * @return 新评分
	 */
	virtual float UpdateRatingWithPerformance(
		float CurrentRating,
		float TeamAverageRating,
		float OpponentAvarageRating,
		bool bWon,
		const FPerformanceMetrics& Performance,
		float Uncertainty,
		int32 GamesPlayed,
		float& OutUncertaintyDelta
	) const = 0;

	/**
	 * @brief 获取动态K因子
	 * @param CurrentRating 当前评分
	 * @param GamesPlayed 已进行对局数
	 * @param Uncertainty 当前不确定性
	 * @return 动态K因子
	 */
	virtual float GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const = 0;

	/**
	 * @brief 获取用于匹配的评分
	 * @param HiddenRating 隐藏分
	 * @param Uncertainty 不确定性
	 * @return 匹配评分（可能包含浮动）
	 */
	virtual float GetMatchmakingRating(float HiddenRating, float Uncertainty) const = 0;
};

/**
 * @brief ELO算法实现类
 * 
 * 实现经典的ELO评分系统：
 * - 预期胜率 = 1 / (1 + 10^((Rb-Ra)/400))
 * - 新评分 = Ra + K * (实际得分 - 预期胜率)
 */
class FELOAlgorithm : public IRatingAlgorithm
{
public:
	/**
	 * @brief 构造函数
	 * @param BaseK 基础K因子
	 * @param DefaultRating 默认评分
	 */
	FELOAlgorithm(float BaseK = 32.f, float DefaultRating = 1500.f);

	virtual float CalculateExpectedScore(float RatingA, float RatingB) const override;
	virtual float UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const override;
	virtual float UpdateRatingWithPerformance(
		float CurrentRating,
		float TeamAverageRating,
		float OpponentAverageRating,
		bool bWon,
		const FPerformanceMetrics& Performance,
		float Uncertainty,
		int32 GamesPlayed,
		float& OutUncertaintyDelta) const override;
	virtual float GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const override;
	virtual float GetMatchmakingRating(float HiddenRating, float Uncertainty) const override;

private:
	float BaseKFactor;
	float DefaultRating;
};

/**
 * @brief MMR算法实现类
 * 
 * 改进的评分系统，特点：
 * - 考虑个人表现分调整评分变化
 * - 不确定性机制，新玩家评分浮动更大
 * - 动态K因子根据评分和不确定性调整
 */
class FMMRAlgorithm : public IRatingAlgorithm
{
public:
	/**
	 * @brief 构造函数
	 * @param BaseK 基础K因子
	 * @param InitialUncertainty 初始不确定性
	 */
	FMMRAlgorithm(float BaseK = 32.f, float InitialUncertainty = 400.0f);

	virtual float CalculateExpectedScore(float RatingA, float RatingB) const override;
	virtual float UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const override;
	virtual float UpdateRatingWithPerformance(
		float CurrentRating,
		float TeamAverageRating,
		float OpponentAverageRating,
		bool bWon,
		const FPerformanceMetrics& Performance,
		float Uncertainty,
		int32 GamesPlayed,
		float& OutUncertaintyDelta) const override;
	virtual float GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const override;
	virtual float GetMatchmakingRating(float HiddenRating, float Uncertainty) const override;

private:
	float BaseKFactor;
	float InitialUncertainty;
};
