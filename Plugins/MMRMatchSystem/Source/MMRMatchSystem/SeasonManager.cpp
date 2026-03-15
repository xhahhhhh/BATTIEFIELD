/**
 * @file SeasonManager.cpp
 * @brief 赛季管理器实现
 */

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
	
	// 检查赛季是否到期
	if (CurrentSeason.bIsActive && Now >= CurrentSeason.EndDate)
	{
		// 标记当前赛季结束
		CurrentSeason.bIsActive = false;
		
		// 广播赛季结束事件
		OnSeasonEnded.Broadcast(CurrentSeason.SeasonNumber);
		
		// 创建新赛季
		CurrentSeason.SeasonNumber++;
		CurrentSeason.StartDate = Now;
		CurrentSeason.EndDate = Now + FTimespan(SeasonDurationDays, 0, 0, 0);
		CurrentSeason.bIsActive = true;
	}
}

float USeasonManager::ApplySoftReset(float CurrentRating, int32 GamesPlayed, float WinRate) const
{
	// 软重置公式：向默认评分靠拢
	// 新评分 = 当前评分 * (1 - 重置强度) + 默认评分 * 重置强度
	float NewRating = CurrentRating * (1.f - SoftResetStrength) + DefaultRating * SoftResetStrength;
	
	// 根据上赛季胜率给予奖励/惩罚
	if (WinRate > 0.55f)
		NewRating += 50.0f;  // 高胜率奖励
	else if (WinRate < 0.45f)
		NewRating -= 50.0f;  // 低胜率惩罚
	
	return NewRating;
}
