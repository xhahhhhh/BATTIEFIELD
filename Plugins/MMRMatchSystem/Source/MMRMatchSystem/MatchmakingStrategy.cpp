/**
 * @file MatchmakingStrategy.cpp
 * @brief 匹配策略实现
 */

#include "MatchmakingStrategy.h"

#include "GameFramework/PlayerState.h"

TArray<FMatchResult> UDefaultMatchmakingStrategy::FindMatches_Implementation(
	const TMap<APlayerController*, FMatchmakingPlayerData>& Queue, 
	int32 TeamSize)
{
	TArray<FMatchResult> Results;
	
	// 人数不足则返回空结果
	if (Queue.Num() < TeamSize * 2) return Results;
	
	// 将队列转换为数组
	TArray<FMatchmakingPlayerData> Players;
	Queue.GenerateValueArray(Players);
	
	// 排序：优先匹配等待时间长的玩家
	// 等待时间相同时，评分低的优先
	Algo::Sort(Players, [](const FMatchmakingPlayerData& A, const FMatchmakingPlayerData& B)
	{
		if (A.WaitTime != B.WaitTime) return A.WaitTime > B.WaitTime;
		return A.Rating < B.Rating;
	});
	
	// 贪心算法：以等待最久的玩家为锚点组建队伍
	while (Players.Num() >= TeamSize * 2)
	{
		// 取等待最久的玩家作为锚点
		FMatchmakingPlayerData Anchor = Players[0];
		Players.RemoveAt(0);
		
		// 组建TeamA（包含锚点玩家）
		TArray<FMatchmakingPlayerData> TeamA;
		TeamA.Add(Anchor);
		
		int32 i = 0;
		// 选择评分相近的玩家加入TeamA
		for (i = 0; Players.Num() && TeamA.Num() < TeamSize; ++i)
		{
			TeamA.Add(Players[i]);
		}
		
		// 从可用玩家列表中移除已加入TeamA的玩家
		for (i = TeamA.Num() - 1; i > 0; --i)
		{
			Players.Remove(TeamA[i]);
		}
		
		// 人数不足则停止匹配
		if (Players.Num() < TeamSize) break;
		
		// 组建TeamB
		TArray<FMatchmakingPlayerData> TeamB;
		for (i = 0; i < TeamSize; ++i)
		{
			TeamB.Add(Players[i]);
		}
		
		// 从可用玩家列表中移除已加入TeamB的玩家
		for (i = TeamSize - 1; i >= 0; --i)
		{
			Players.RemoveAt(i);
		}
		
		// 构建匹配结果
		FMatchResult Result;
		for (const auto& PlayerData : TeamA) Result.TeamA.Add(PlayerData.Player);
		for (const auto& PlayerData : TeamB) Result.TeamB.Add(PlayerData.Player);
		Result.BalanceScore = EvaluateMatchQuality(Result);
		
		Results.Add(Result);
	}
	
	return Results;
}

float UDefaultMatchmakingStrategy::EvaluateMatchQuality_Implementation(const FMatchResult& Match) const
{
	// 计算两队平均评分
	if (Match.TeamA.Num() == 0 || Match.TeamB.Num() == 0) return 0.f;
	
	float TeamAAverageScore = 0;
	float TeamBAverageScore = 0;
	
	for (auto& Player : Match.TeamA)
	{
		if (Player->GetPlayerState<APlayerState>())
		{
			TeamAAverageScore += Player->GetPlayerState<APlayerState>()->GetScore();
		}
	}
	
	for (auto& Player : Match.TeamB)
	{
		if (Player->GetPlayerState<APlayerState>())
		{
			TeamBAverageScore += Player->GetPlayerState<APlayerState>()->GetScore();
		}
	}
	
	float AvgA = TeamAAverageScore / Match.TeamA.Num();
	float AvgB = TeamBAverageScore / Match.TeamB.Num();
	
	// 返回两队平均分的均值作为质量分数
	return (AvgA + AvgB) / 2;
}
