/**
 * @file RatingAlgorithm.cpp
 * @brief 评分算法实现
 */

#include "RatingAlgorithm.h"

//============================
// ELO算法实现
//============================

FELOAlgorithm::FELOAlgorithm(float BaseK, float DefaultRating)
	: BaseKFactor(BaseK), DefaultRating(DefaultRating)
{
}

float FELOAlgorithm::CalculateExpectedScore(float RatingA, float RatingB) const
{
	// ELO预期胜率公式：1 / (1 + 10^((Rb-Ra)/400))
	return 1.f / (1.f + FMath::Pow(10.f, (RatingB - RatingA) / 400.f));
}

float FELOAlgorithm::UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const
{
	float Expected = CalculateExpectedScore(CurrentRating, OpponentRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = (KFactor > 0) ? KFactor : GetDynamicKFactor(CurrentRating, 0, 0);
	
	// ELO更新公式：Ra + K * (实际得分 - 预期胜率)
	return CurrentRating + K * (Actual - Expected);
}

float FELOAlgorithm::UpdateRatingWithPerformance(
	float CurrentRating, 
	float TeamAverageRating,
	float OpponentAverageRating, 
	bool bWon, 
	const FPerformanceMetrics& Performance, 
	float Uncertainty,
	int32 GamesPlayed, 
	float& OutUncertaintyDelta) const
{
	// ELO算法不考虑个人表现，委托给基础版本
	OutUncertaintyDelta = 0.0f;
	return UpdateRating(CurrentRating, OpponentAverageRating, bWon, -1.0f);
}

float FELOAlgorithm::GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const
{
	// 新玩家使用更高的K因子（快速定位真实水平）
	if (GamesPlayed < 50) return BaseKFactor * 2.f;
	
	// 高分玩家使用较低的K因子（保持稳定性）
	if (CurrentRating > 1500) return BaseKFactor * 0.5f;
	
	return BaseKFactor;
}

float FELOAlgorithm::GetMatchmakingRating(float HiddenRating, float Uncertainty) const
{
	// ELO算法直接使用隐藏分
	return HiddenRating;
}

//============================
// MMR算法实现
//============================

FMMRAlgorithm::FMMRAlgorithm(float BaseK, float InitialUncertainty)
	: BaseKFactor(BaseK), InitialUncertainty(InitialUncertainty)
{
}

float FMMRAlgorithm::CalculateExpectedScore(float RatingA, float RatingB) const
{
	// 与ELO相同的预期胜率计算
	return 1.f / (1.f + FMath::Pow(10.f, (RatingB - RatingA) / 400.f));
}

float FMMRAlgorithm::UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const
{
	float Expected = CalculateExpectedScore(CurrentRating, OpponentRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = (KFactor > 0) ? KFactor : GetDynamicKFactor(CurrentRating, 0, InitialUncertainty);
	
	return CurrentRating + K * (Actual - Expected);
}

float FMMRAlgorithm::UpdateRatingWithPerformance(
	float CurrentRating,
	float TeamAverageRating,
	float OpponentAverageRating,
	bool bWon,
	const FPerformanceMetrics& Performance,
	float Uncertainty,
	int32 GamesPlayed,
	float& OutUncertaintyDelta) const
{
	// 计算基础评分变化（与ELO相同）
	float Expected = CalculateExpectedScore(TeamAverageRating, OpponentAverageRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = GetDynamicKFactor(CurrentRating, GamesPlayed, Uncertainty);
	
	// 计算个人表现因子（0.8 - 1.5之间）
	float PerformanceFactor = 1.f;
	if (GamesPlayed > 0)
	{
		float PerfScore = Performance.GetOverallPerformance();
		PerformanceFactor = 0.8f + PerfScore * 0.7f;
	}
	
	// 表现分影响评分变化
	float BaseChange = K * (Actual - Expected);
	float TotalChange = BaseChange * PerformanceFactor;
	
	// 不确定性每次衰减2%
	OutUncertaintyDelta = -Uncertainty * 0.02f;
	
	return CurrentRating + TotalChange;
}

float FMMRAlgorithm::GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const
{
	// 基础K因子
	float K = BaseKFactor;
	
	// 新玩家快速定位
	if (GamesPlayed < 50) return BaseKFactor * 2.f;
	
	// 高分段降低K因子
	if (CurrentRating > 2000) K *= 0.7f;
	if (CurrentRating > 2500) K *= 0.5f;
	
	// 不确定性越高，K因子越大（允许更大浮动）
	K *= (Uncertainty / InitialUncertainty);
	
	// 限制K因子范围
	return FMath::Clamp(K, 10.f, 100.f);
}

float FMMRAlgorithm::GetMatchmakingRating(float HiddenRating, float Uncertainty) const
{
	// 根据不确定性添加随机浮动
	// 不确定性高的玩家匹配范围更大
	float Range = Uncertainty * 0.5f;
	return HiddenRating + FMath::RandRange(-Range, Range);
}
