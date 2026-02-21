// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchmakingStrategy.h"

#include "GameFramework/PlayerState.h"

TArray<FMatchResult> UDefaultMatchmakingStrategy::FindMatches_Implementation(
	const TMap<APlayerController*, FMatchmakingPlayerData>& Queue, int32 TeamSize)
{
	TArray<FMatchResult> Results;
	
	if (Queue.Num() < TeamSize*2) return Results;
	
	//使用队列构建玩家数组
	TArray<FMatchmakingPlayerData> Players;
	Queue.GenerateValueArray(Players);
	
	//通过玩家匹配时间和评分对数组升序排序
	Algo::Sort(Players,[](const FMatchmakingPlayerData& A, const FMatchmakingPlayerData& B)
	{
		if (A.WaitTime != B.WaitTime) return A.WaitTime > B.WaitTime;
		return A.Rating < B.Rating;
	});
	
	//利用贪心思想将等待时间最久的玩家作为锚点
	while (Players.Num() >= TeamSize*2)
	{
		FMatchmakingPlayerData Ancher = Players[0];
		Players.RemoveAt(0);
		
		//寻找与锚点玩家评分相近的玩家组成团队
		TArray<FMatchmakingPlayerData> TeamA;
		TeamA.Add(Ancher);
		int32 i = 0;
		//构建TeamA玩家并将已加入TeamA的玩家移除匹配队列
		for (i =0;Players.Num() && TeamA.Num() < TeamSize; ++i)
		{
			TeamA.Add(Players[i]);
		}
		
		for (i = TeamA.Num()-1;i > 0;--i)
		{
			Players.Remove(TeamA[i]);
		}
		
		if (Players.Num() < TeamSize) break;
		
		//筛选剩下的玩家构建TeamB
		TArray<FMatchmakingPlayerData> TeamB;
		for (i =0;i<TeamSize; ++i)
		{
			TeamA.Add(Players[i]);
		}
		for (i = TeamSize - 1; i >= 0; --i)
		{
			Players.RemoveAt(i);
		}
		
		//构建对局结果
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
	//计算团队平均评分
	if (Match.TeamA.Num() == 0 || Match.TeamB.Num() == 0) return 0.f;
	int TeamAAverageScore = 0;
	int TeamBAverageScore = 0;
	for (auto& Player : Match.TeamA)TeamAAverageScore+=Player->GetPlayerState<APlayerState>()->GetScore();
	for (auto& Player : Match.TeamB)TeamBAverageScore+=Player->GetPlayerState<APlayerState>()->GetScore();
	return ((TeamAAverageScore/Match.TeamA.Num()) +(TeamAAverageScore/Match.TeamA.Num()))/2;
}
