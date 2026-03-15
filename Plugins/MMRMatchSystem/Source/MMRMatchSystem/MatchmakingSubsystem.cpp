/**
 * @file MatchmakingSubsystem.cpp
 * @brief 匹配子系统实现
 */

#include "MatchmakingSubsystem.h"

#include "PlayerRatingComponent.h"

void UMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 创建默认匹配策略
	if (DefaultStrategyClass)
	{
		UObject* StrategyObj = NewObject<UObject>(this, DefaultStrategyClass);
		if (StrategyObj && StrategyObj->GetClass()->ImplementsInterface(UMatchmakingStrategy::StaticClass()))
		{
			CurrentStrategy.SetObject(StrategyObj);
			CurrentStrategy.SetInterface(Cast<IMatchmakingStrategy>(StrategyObj));
		}
	}
	
	// 如果没有默认策略，创建内置默认策略
	if (!CurrentStrategy.GetInterface())
	{
		UDefaultMatchmakingStrategy* DefaultStrategy = NewObject<UDefaultMatchmakingStrategy>(this);
		if (DefaultStrategy)
		{
			CurrentStrategy.SetObject(DefaultStrategy);
			CurrentStrategy.SetInterface(DefaultStrategy);
		}
	}
	
	// 设置定时器定期执行匹配
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			MatchmakingTimerHandle,
			this,
			&UMatchmakingSubsystem::ProcessMatchmaking,
			MatchmakingInterval,
			true  // 循环执行
		);
	}
}

void UMatchmakingSubsystem::Deinitialize()
{
	// 清除定时器
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(MatchmakingTimerHandle);
	}
	
	// 清空匹配队列
	Queue.Empty();
	
	Super::Deinitialize();
}

void UMatchmakingSubsystem::AddToQueue(APlayerController* Player, const FMatchmakingPreferences& Preferences)
{
	if (!Player) return;
	
	// 获取玩家评分组件
	UPlayerRatingComponent* RatingComp = Player->FindComponentByClass<UPlayerRatingComponent>();
	if (!RatingComp) return;
	
	// 创建玩家匹配数据
	FMatchmakingPlayerData Data;
	Data.Player = Player;
	Data.Rating = RatingComp->GetMatchmakingRating();
	Data.Uncertainty = RatingComp->GetUncertainty();
	Data.WaitTime = 0.f;
	Data.PreferredRoles = Preferences.PreferredRoles;
	Data.WinStreak = RatingComp->GetWinStreak();
	
	// 加入队列
	Queue.Add(Player, Data);
}

void UMatchmakingSubsystem::RemoveFromQueue(APlayerController* Player)
{
	Queue.Remove(Player);
}

void UMatchmakingSubsystem::ProcessMatchmaking()
{
	// 人数不足两队则返回
	if (Queue.Num() < TargetTeamSize * 2) return;
	
	if (!CurrentStrategy.GetInterface()) return;
	
	// 调用策略寻找匹配
	TArray<FMatchResult> Matches = CurrentStrategy->Execute_FindMatches(
		CurrentStrategy.GetObject(),
		Queue,
		TargetTeamSize
	);
	
	// 处理匹配结果
	for (const FMatchResult& Match : Matches)
	{
		// 验证所有玩家是否仍在队列中
		bool bAllInQueue = true;
		
		for (APlayerController* PlayerA : Match.TeamA)
		{
			if (!Queue.Contains(PlayerA))
			{
				bAllInQueue = false;
				break;
			}
		}
		
		for (APlayerController* PlayerB : Match.TeamB)
		{
			if (!Queue.Contains(PlayerB))
			{
				bAllInQueue = false;
				break;
			}
		}
		
		if (!bAllInQueue) continue;
		
		// 将匹配成功的玩家移出队列
		for (APlayerController* Player : Match.TeamA) Queue.Remove(Player);
		for (APlayerController* Player : Match.TeamB) Queue.Remove(Player);
		
		// TODO: 创建对局，通知玩家
	}
}

void UMatchmakingSubsystem::SetMatchmakingStrategy(TScriptInterface<IMatchmakingStrategy> NewStrategy)
{
	CurrentStrategy = NewStrategy;
}
