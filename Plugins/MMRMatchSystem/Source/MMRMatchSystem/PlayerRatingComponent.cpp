#include "PlayerRatingComponent.h"
#include "PerformanceMetrics.h"
#include "RatingConfigDataAsset.h"

UPlayerRatingComponent::UPlayerRatingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	RatingData.LastUpdateTime = FDateTime::Now();
}



void UPlayerRatingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (RatingConfig)
	{
		ConfigAlgorithmInstance();
	}
	else
	{
		AlgorithmInstance = MakeUnique<FMMRAlgorithm>(32.f,400.f);
	}
}

void UPlayerRatingComponent::SetRatingAlgorithm(ERatingAlgorithmType NewAlgorithmType)
{
	if (AlgorithmType == NewAlgorithmType) return;
	AlgorithmType = NewAlgorithmType;
	if (RatingConfig)
	{
		ConfigAlgorithmInstance();
	}
}

void UPlayerRatingComponent::UpdateRatingForTeamGame(float TeamAverageRating, float OpponentAverageRating, bool bWon,
                                                     const FPerformanceMetrics& Performance, const FString& Role)
{
	if (!AlgorithmInstance.IsValid())return;
	
	float NewRating = RatingData.HiddenRating;
	float NewUncertainty = RatingData.Uncertainty;
	float RatingChange = 0.f;
	
	if (AlgorithmType == ERatingAlgorithmType::MMR)//MMR
	{
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
		NewUncertainty = FMath::Max(RatingConfig?RatingConfig->MMRConfig.MinUncertainty : 50.f,RatingData.Uncertainty * (RatingConfig?RatingConfig->MMRConfig.UncertaintyDecay : 0.98f));
		RatingChange = NewRating - RatingData.HiddenRating;
	}
	else//ELO
	{
		NewRating = AlgorithmInstance->UpdateRating(
			RatingData.HiddenRating,
			OpponentAverageRating,
			bWon);
		RatingChange = NewRating - RatingData.HiddenRating;
	}
	
	RatingData.HiddenRating = NewRating;
	RatingData.Uncertainty = NewUncertainty;
	RatingData.GamesPlayed++;
	
	if (bWon) RatingData.Wins++;
	RatingData.RatingHistory.Add(NewRating);
	if (RatingData.RatingHistory.Num() > 50) RatingData.RatingHistory.RemoveAt(0);
	
	UpdateStreak(bWon);
	
	if (!Role.IsEmpty() && AlgorithmType == ERatingAlgorithmType::MMR)
	{
		float &Proficiency = RatingData.RoleProficiency.FindOrAdd(Role,.5f);
		
		float PerfScore = Performance.GetOverallPerformance();
		Proficiency = FMath::Clamp(Proficiency * 0.9f + PerfScore * .1f, 0.0f, 1.0f);
	}
	
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
	
	float NewRating = AlgorithmInstance->UpdateRating(
		RatingData.HiddenRating,
		OpponentRating,
		bWon,
		KFactor);
	
	RatingData.HiddenRating = NewRating;
	RatingData.GamesPlayed++;
	if (bWon) RatingData.Wins++;
	RatingData.RatingHistory.Add(NewRating);
	if (RatingData.RatingHistory.Num() > 50) RatingData.RatingHistory.RemoveAt(0);
	UpdateStreak(bWon);
	RatingData.LastUpdateTime = FDateTime::Now();
}

float UPlayerRatingComponent::GetMatchmakingRating() const
{
	if (!AlgorithmInstance.IsValid()) return RatingData.HiddenRating;
	return AlgorithmInstance->GetMatchmakingRating(RatingData.HiddenRating,RatingData.Uncertainty);
}

FString UPlayerRatingComponent::GetVisibleRank() const
{
	if (!RatingConfig) return TEXT("");
	FRankTier Tier = RatingConfig->GetTierForRating(RatingData.HiddenRating);
	return Tier.Name;
}

FLinearColor UPlayerRatingComponent::GetRankColor() const
{
	if (!RatingConfig) return FLinearColor::Green;
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