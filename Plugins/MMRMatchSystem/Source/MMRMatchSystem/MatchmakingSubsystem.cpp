
#include "MatchmakingSubsystem.h"

#include "PlayerRatingComponent.h"

void UMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	//创建默认匹配策略
	if (DefaultStrategyClass)
	{
		UObject* StrategyObj = NewObject<UObject>(this, DefaultStrategyClass);
		if (StrategyObj && StrategyObj->GetClass()->ImplementsInterface(UMatchmakingStrategy::StaticClass()))
		{
			CurrentStrategy.SetObject(StrategyObj);
			CurrentStrategy.SetInterface(Cast<IMatchmakingStrategy>(StrategyObj));
		}
	}
	
	//若没有则重新设置
	if (!CurrentStrategy.GetInterface())
	{
		UDefaultMatchmakingStrategy* DefaultStrategy = NewObject<UDefaultMatchmakingStrategy>(this);
		if (DefaultStrategy)
		{
			CurrentStrategy.SetObject(DefaultStrategy);
			CurrentStrategy.SetInterface(DefaultStrategy);
		}
	}
	
	//设置定时器进行匹配
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(MatchmakingTimerHandle,this,&UMatchmakingSubsystem::ProcessMatchmaking,MatchmakingInterval,true);
		
	}
	
}

void UMatchmakingSubsystem::Deinitialize()
{
	//销毁子系统实例时清除定时器并清除队列
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(MatchmakingTimerHandle);
	}
	
	Queue.Empty();
	Super::Deinitialize();
	
}

void UMatchmakingSubsystem::AddToQueue(APlayerController* Player, const FMatchmakingPreferences& Preferences)
{
	if (!Player)return;
	
	//获取玩家评分组件
	UPlayerRatingComponent* RatingComp = Player->FindComponentByClass<UPlayerRatingComponent>();
	if (!RatingComp)return;
	
	//创建玩家对局数据并加入匹配队列
	FMatchmakingPlayerData Data;
	Data.Player = Player;
	Data.Rating = RatingComp->GetMatchmakingRating();
	Data.Uncertainty = RatingComp->GetUncertainty();
	Data.WaitTime = 0.f;
	Data.PreferredRoles = Preferences.PreferredRoles;
	Data.WinStreak = RatingComp->GetWinStreak();
	
	Queue.Add(Player,Data);
	
}

//将玩家移出匹配队列
void UMatchmakingSubsystem::RemoveFromQueue(APlayerController* Player)
{
	Queue.Remove(Player);
}

//匹配中执行操作
void UMatchmakingSubsystem::ProcessMatchmaking()
{
	//若少于两队人则返回
	if (Queue.Num() < TargetTeamSize * 2) return; 
	
	if (!CurrentStrategy.GetInterface()) return;
	
	//调用创建的策略寻找对局
	TArray<FMatchResult> Matches = CurrentStrategy->Execute_FindMatches(CurrentStrategy.GetObject(),Queue,TargetTeamSize);
	
	//处理匹配结果
	for (const FMatchResult& Match : Matches)
	{
		//验证玩家是否在队列
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
		if (!bAllInQueue)continue;
		
		//将玩家移除队列
		for (APlayerController* Player : Match.TeamA) Queue.Remove(Player);
		for (APlayerController* Player : Match.TeamB) Queue.Remove(Player);
	}
}

void UMatchmakingSubsystem::SetMatchmakingStrategy(TScriptInterface<IMatchmakingStrategy> NewStrategy)
{
	CurrentStrategy = NewStrategy;
}
