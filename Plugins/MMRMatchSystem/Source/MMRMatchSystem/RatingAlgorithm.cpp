#include "RatingAlgorithm.h"

//ELO构造初始化K和默认对局
FELOAlgorithm::FELOAlgorithm(float BaseK, float DefaultRating):BaseKFactor(BaseK),DefaultRating(DefaultRating)
{
	
}

float FELOAlgorithm::CalculateExpectedScore(float RatingA, float RatingB) const
{
	//获取期望得分 = 1/(1+10^(rb-ra)/400))
	return 1.f/(1.f + FMath::Pow(10.f,(RatingB - RatingA)/400.f));
}

float FELOAlgorithm::UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const
{
	float Expected = CalculateExpectedScore(CurrentRating, OpponentRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = (KFactor >0) ? KFactor : GetDynamicKFactor(CurrentRating,0,0);
	
	//更新评分 = Ra + k * (bwon - 期望得分)
	return CurrentRating + K*(Actual - Expected);
}

float FELOAlgorithm::UpdateRatingWithPerformance(float CurrentRating, float TeamAverageRating,
	float OpponentAverageRating, bool bWon, const FPerformanceMetrics& Performance, float Uncertainty,
	int32 GamesPlayed, float& OutUncertaintyDelta) const
{
	OutUncertaintyDelta = 0.0f;
	return UpdateRating(CurrentRating, OpponentAverageRating, bWon, -1.0f);
}

//获取动态K因子，根据对局场数以及评分而定
float FELOAlgorithm::GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const
{
	if (GamesPlayed<50)return BaseKFactor * 2.f;
	if (CurrentRating>1500)return BaseKFactor * .5f;
	return BaseKFactor;
}

float FELOAlgorithm::GetMatchmakingRating(float HiddenRating, float Uncertainty) const
{
	return HiddenRating;
}

//MMR构造初始化K和默认对局
FMMRAlgorithm::FMMRAlgorithm(float BaseK, float InitialUncertainty):BaseKFactor(BaseK),InitialUncertainty(InitialUncertainty)
{
}

float FMMRAlgorithm::CalculateExpectedScore(float RatingA, float RatingB) const
{
	return 1.f/(1.f + FMath::Pow(10.f,(RatingB - RatingA)/400.f));
}

float FMMRAlgorithm::UpdateRating(float CurrentRating, float OpponentRating, bool bWon, float KFactor) const
{
	float Expected = CalculateExpectedScore(CurrentRating, OpponentRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = (KFactor >0) ? KFactor : GetDynamicKFactor(CurrentRating,0,InitialUncertainty);
	return CurrentRating + K*(Actual - Expected);
}

float FMMRAlgorithm::UpdateRatingWithPerformance(float CurrentRating, float TeamAverageRating,
	float OpponentAverageRating, bool bWon, const FPerformanceMetrics& Performance, float Uncertainty,
	int32 GamesPlayed, float& OutUncertaintyDelta) const
{
	//与ELO同理
	float Expected = CalculateExpectedScore(TeamAverageRating,OpponentAverageRating);
	float Actual = bWon ? 1.f : 0.f;
	float K = GetDynamicKFactor(CurrentRating, GamesPlayed, Uncertainty);
	
	//计算对局表现分
	float PerformanceFactor = 1.f;
	if (GamesPlayed>0)
	{
		float PerfScore = Performance.GetOverallPerformance();
		PerformanceFactor = 0.8f + PerfScore*0.7f;//限制到0.8-1.5
	}
	
	//利用对局表现影响分数变化
	float BaseChange = K * (Actual - Expected);
	float TotalChange = BaseChange * PerformanceFactor;
	
	//每次对局Uncertainty衰减2%
	OutUncertaintyDelta = -Uncertainty * 0.02f;
	
	//更新评分
	return CurrentRating + TotalChange;
}

float FMMRAlgorithm::GetDynamicKFactor(float CurrentRating, int32 GamesPlayed, float Uncertainty) const
{
	//获取动态K因子，根据对局场数以及评分而定，并限制到10-100
	float K = BaseKFactor;
	if (GamesPlayed<50)return BaseKFactor * 2.f;
	
	if (CurrentRating > 2000) K *= 0.7f;
	if (CurrentRating > 2500) K *= 0.5f;
	
	//计算不确定性比率对K值的影响
	K*=(Uncertainty/InitialUncertainty);
	
	return FMath::Clamp(K,10.f,100.f);
}

float FMMRAlgorithm::GetMatchmakingRating(float HiddenRating, float Uncertainty) const
{
	//通过不确定性改变匹配浮动范围
	float Range = Uncertainty * .5f;
	return HiddenRating + FMath::RandRange(-Range,Range);
}
