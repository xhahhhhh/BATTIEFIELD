#pragma once

#include "CoreMinimal.h"
#include "PerformanceMetrics.h"

class IRatingAlgorithm
{
public:
	virtual ~IRatingAlgorithm()  = default;
	
	//计算预期胜率
	virtual float CalculateExpectedScore(float RatingA,float RatingB) const = 0;
	
	//更新评分（纯ELO）
	virtual float UpdateRating(float CurrentRating,float OpponentRating,bool bWon,float KFactor = -1.f) const = 0;
	
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
	)const;
	
	virtual float GetDynamicKFactor(float CurrentRating,int32 GamesPlayed,float Uncertainty)const = 0;
	
	virtual float GetMatchmakingRating(float HiddenRating,float Uncertainty) = 0;
};

