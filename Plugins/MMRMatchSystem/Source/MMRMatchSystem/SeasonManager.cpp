
#include "SeasonManager.h"

void USeasonManager::Initialize(int32 Season, const FDateTime& Start, const FDateTime& End)
{
	CurrentSeason.SeasonNumber = Season;
	CurrentSeason.StartDate = Start;
	CurrentSeason.EndDate = End;
	CurrentSeason.bIsActive = true;
	
}

void USeasonManager::UpdateSeason()
{
	FDateTime Now = FDateTime::Now();
	if (CurrentSeason.bIsActive && Now >= CurrentSeason.EndDate)
	{
		CurrentSeason.bIsActive = false;
		OnSeasonEnded.Broadcast(CurrentSeason.SeasonNumber);
		
		CurrentSeason.SeasonNumber++;
		CurrentSeason.StartDate = Now;
		CurrentSeason.EndDate = Now + FTimespan(SeasonDurationDays, 0, 0, 0);
		CurrentSeason.bIsActive = true;
	}
}

float USeasonManager::ApplySoftReset(float CurrentRating, int32 GamesPlayed, float WinRate) const
{
	float NewRating = CurrentRating * (1.f - SoftResetStrength) + DefaultRating * SoftResetStrength;
	
	if (WinRate > 0.55f)
		NewRating += 50.0f;
	else if (WinRate < 0.45f)
		NewRating -= 50.0f;
	return NewRating;
}
