#pragma once

#include "CoreMinimal.h"
#include "PerformanceMetrics.h"

class IRatingAlgorithm
{
public:
	virtual ~IRatingAlgorithm() = default;

	//计算预期胜率
	virtual float CalculateExpectedScore(float RatingA, float RatingB) const = 0;

	//更新评分（纯ELO）
	virtual float UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor = -1.f) const = 0;

	//更新评分（MMR实现）
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

	virtual float GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const = 0;

	virtual float GetMatchmakingRating(float HiddenRating, float Uncertainty) const = 0;
};

class FELOAlgorithm : public IRatingAlgorithm
{
public:
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

class FMMRAlgorithm : public IRatingAlgorithm
{
public:
	FMMRAlgorithm(float BaseK = 32.f, float InitialUncertainty = 400.0f);
	virtual float CalculateExpectedScore(float RatingA, float RatingB) const override;
	virtual float UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const override;
	virtual float UpdateRatingWithPerformance(float CurrentRating,
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
