/**
 * @file PlayerRatingComponent.cpp
 * @brief 玩家评分组件实现
 */

#include "PlayerRatingComponent.h"
#include "PerformanceMetrics.h"
#include "RatingConfigDataAsset.h"

UPlayerRatingComponent::UPlayerRatingComponent()
{
	// 禁用Tick，评分计算是事件驱动的
	PrimaryComponentTick.bCanEverTick = false;
	
	// 初始化最后更新时间
	RatingData.LastUpdateTime = FDateTime::Now();
}

void UPlayerRatingComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化匹配算法
	if (RatingConfig)
	{
		ConfigAlgorithmInstance();
	}
	else
	{
		// 默认创建MMR算法（K因子32，初始不确定性400）
		AlgorithmInstance = MakeUnique<FMMRAlgorithm>(32.f, 400.f);
	}
}

void UPlayerRatingComponent::SetRatingAlgorithm(ERatingAlgorithmType NewAlgorithmType)
{
	// 设置算法类型
	if (AlgorithmType == NewAlgorithmType) return;
	
	AlgorithmType = NewAlgorithmType;
	if (RatingConfig)
	{
		ConfigAlgorithmInstance();
	}
}

void UPlayerRatingComponent::UpdateRatingForTeamGame(
	float TeamAverageRating, 
	float OpponentAverageRating, 
	bool bWon,
	const FPerformanceMetrics& Performance, 
	const FString& Role)
{
	if (!AlgorithmInstance.IsValid()) return;
	
	float NewRating = RatingData.HiddenRating;
	float NewUncertainty = RatingData.Uncertainty;
	float RatingChange = 0.f;
	
	// 根据算法类型选择计算方式
	if (AlgorithmType == ERatingAlgorithmType::MMR)
	{
		// MMR算法：考虑个人表现
		float UncertaintyDelta = 0.f;
		NewRating = AlgorithmInstance->UpdateRatingWithPerformance(
			RatingData.HiddenRating,
			TeamAverageRating,
			OpponentAverageRating,
			bWon,
			Performance,
			RatingData.Uncertainty,
			RatingData.GamesPlayed,
			UncertaintyDelta
		);
		
		// 不确定性衰减（每次对局减少2%）
		float MinUncertainty = RatingConfig ? RatingConfig->MMRConfig.MinUncertainty : 50.f;
		float DecayRate = RatingConfig ? RatingConfig->MMRConfig.UncertaintyDecay : 0.98f;
		NewUncertainty = FMath::Max(MinUncertainty, RatingData.Uncertainty * DecayRate);
		
		RatingChange = NewRating - RatingData.HiddenRating;
	}
	else
	{
		// ELO算法：不考虑个人表现
		NewRating = AlgorithmInstance->UpdateRating(
			RatingData.HiddenRating,
			OpponentAverageRating,
			bWon);
		RatingChange = NewRating - RatingData.HiddenRating;
	}
	
	// 更新评分数据
	RatingData.HiddenRating = NewRating;
	RatingData.Uncertainty = NewUncertainty;
	RatingData.GamesPlayed++;
	
	if (bWon) RatingData.Wins++;
	
	// 记录评分历史（最多保留50条）
	RatingData.RatingHistory.Add(NewRating);
	if (RatingData.RatingHistory.Num() > 50) RatingData.RatingHistory.RemoveAt(0);
	
	UpdateStreak(bWon);
	
	// 更新角色熟练度（仅MMR算法）
	if (!Role.IsEmpty() && AlgorithmType == ERatingAlgorithmType::MMR)
	{
		float& Proficiency = RatingData.RoleProficiency.FindOrAdd(Role, 0.5f);
		
		float PerfScore = Performance.GetOverallPerformance();
		Proficiency = FMath::Clamp(Proficiency * 0.9f + PerfScore * 0.1f, 0.0f, 1.0f);
	}
	
	// 更新最近表现记录
	if (AlgorithmType == ERatingAlgorithmType::MMR)
	{
		RatingData.RecentPerformances.Add(Performance.GetOverallPerformance());
		if (RatingData.RecentPerformances.Num() > 20) RatingData.RecentPerformances.RemoveAt(0);
	}
	
	RatingData.LastUpdateTime = FDateTime::Now();
}

void UPlayerRatingComponent::UpdateRatingForDuel(float OpponentRating, bool bWon, float KFactor)
{
	if (!AlgorithmInstance.IsValid()) return;
	
	// 计算新评分
	float NewRating = AlgorithmInstance->UpdateRating(
		RatingData.HiddenRating,
		OpponentRating,
		bWon,
		KFactor);
	
	// 更新数据
	RatingData.HiddenRating = NewRating;
	RatingData.GamesPlayed++;
	if (bWon) RatingData.Wins++;
	
	// 记录评分历史
	RatingData.RatingHistory.Add(NewRating);
	if (RatingData.RatingHistory.Num() > 50) RatingData.RatingHistory.RemoveAt(0);
	
	UpdateStreak(bWon);
	RatingData.LastUpdateTime = FDateTime::Now();
}

float UPlayerRatingComponent::GetMatchmakingRating() const
{
	if (!AlgorithmInstance.IsValid()) return RatingData.HiddenRating;
	
	// 获取用于匹配的评分（考虑不确定性浮动）
	return AlgorithmInstance->GetMatchmakingRating(RatingData.HiddenRating, RatingData.Uncertainty);
}

FString UPlayerRatingComponent::GetVisibleRank() const
{
	if (!RatingConfig) return TEXT("");
	
	// 根据评分获取对应段位
	FRankTier Tier = RatingConfig->GetTierForRating(RatingData.HiddenRating);
	return Tier.Name;
}

FLinearColor UPlayerRatingComponent::GetRankColor() const
{
	if (!RatingConfig) return FLinearColor::Green;
	
	// 获取段位颜色
	FRankTier Tier = RatingConfig->GetTierForRating(RatingData.HiddenRating);
	return Tier.Color;
}

void UPlayerRatingComponent::UpdateStreak(bool bWon)
{
	if (bWon)
	{
		RatingData.WinStreak++;
		RatingData.LoseStreak = 0;
	}
	else
	{
		RatingData.LoseStreak++;
		RatingData.WinStreak = 0;
	}
}

void UPlayerRatingComponent::ConfigAlgorithmInstance()
{
	// 根据配置创建对应的算法实例
	switch (AlgorithmType)
	{
	case ERatingAlgorithmType::ELO:
		AlgorithmInstance = MakeUnique<FELOAlgorithm>(
			RatingConfig->ELOConfig.BaseKFactor,
			RatingConfig->DefaultRating);
		break;
		
	case ERatingAlgorithmType::MMR:
		AlgorithmInstance = MakeUnique<FMMRAlgorithm>(
			RatingConfig->MMRConfig.BaseKFactor,
			RatingConfig->MMRConfig.InitialUncertainty);
		break;
		
	default:
		break;
	}
}
