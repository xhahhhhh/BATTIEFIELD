// Fill out your copyright notice in the Description page of Project Settings.


#include "MatchmakingStrategy.h"

#include "GameFramework/PlayerState.h"


// Add default functionality here for any IMatchmakingStrategy functions that are not pure virtual.
TArray<FMatchResult> UDefaultMatchmakingStrategy::FindMatches_Implementation(
	const TMap<APlayerController*, FMatchmakingPlayerData>& Queue, int32 TeamSize)
{
	TArray<FMatchResult> Results;
	
	if (Queue.Num() < TeamSize*2) return Results;
	
	TArray<FMatchmakingPlayerData> Players;
	Queue.GenerateValueArray(Players);
	Algo::Sort(Players,[](const FMatchmakingPlayerData& A, const FMatchmakingPlayerData& B)
	{
		if (A.WaitTime != B.WaitTime) return A.WaitTime > B.WaitTime;
		return A.Rating < B.Rating;
	});
	
	while (Players.Num() >= TeamSize*2)
	{
		FMatchmakingPlayerData Ancher = Players[0];
		Players.RemoveAt(0);
		
		TArray<FMatchmakingPlayerData> TeamA;
		TeamA.Add(Ancher);
		int32 i = 0;
		for (i =0;Players.Num() && TeamA.Num() < TeamSize; ++i)
		{
			TeamA.Add(Players[i]);
		}
		
		for (i = TeamA.Num()-1;i > 0;--i)
		{
			Players.Remove(TeamA[i]);
		}
		
		if (Players.Num() < TeamSize) break;
		
		TArray<FMatchmakingPlayerData> TeamB;
		for (i =0;i<TeamSize; ++i)
		{
			TeamA.Add(Players[i]);
		}
		for (i = TeamSize - 1; i >= 0; --i)
		{
			Players.RemoveAt(i);
		}
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
	if (Match.TeamA.Num() == 0 || Match.TeamB.Num() == 0) return 0.f;
	int TeamAAverageScore = 0;
	int TeamBAverageScore = 0;
	for (auto& Player : Match.TeamA)TeamAAverageScore+=Player->GetPlayerState<APlayerState>()->GetScore();
	for (auto& Player : Match.TeamB)TeamBAverageScore+=Player->GetPlayerState<APlayerState>()->GetScore();
	return ((TeamAAverageScore/Match.TeamA.Num()) +(TeamAAverageScore/Match.TeamA.Num()))/2;
}
