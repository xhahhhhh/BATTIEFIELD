
#include "MatchmakingSubsystem.h"

#include "PlayerRatingComponent.h"

void UMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (DefaultStrategyClass)
	{
		UObject* StrategyObj = NewObject<UObject>(this, DefaultStrategyClass);
		if (StrategyObj && StrategyObj->GetClass()->ImplementsInterface(UMatchmakingStrategy::StaticClass()))
		{
			CurrentStrategy.SetObject(StrategyObj);
			CurrentStrategy.SetInterface(Cast<IMatchmakingStrategy>(StrategyObj));
		}
	}
	
	if (!CurrentStrategy.GetInterface())
	{
		UDefaultMatchmakingStrategy* DefaultStrategy = NewObject<UDefaultMatchmakingStrategy>(this);
		if (DefaultStrategy)
		{
			CurrentStrategy.SetObject(DefaultStrategy);
			CurrentStrategy.SetInterface(DefaultStrategy);
		}
	}
	
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(MatchmakingTimerHandle,this,&UMatchmakingSubsystem::ProcessMatchmaking,MatchmakingInterval,true);
		
	}
	
}

void UMatchmakingSubsystem::Deinitialize()
{
	
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
	
	UPlayerRatingComponent* RatingComp = Player->FindComponentByClass<UPlayerRatingComponent>();
	if (!RatingComp)return;
	
	FMatchmakingPlayerData Data;
	Data.Player = Player;
	Data.Rating = RatingComp->GetMatchmakingRating();
	Data.Uncertainty = RatingComp->GetUncertainty();
	Data.WaitTime = 0.f;
	Data.PreferredRoles = Preferences.PreferredRoles;
	Data.WinStreak = RatingComp->GetWinStreak();
	
	Queue.Add(Player,Data);
	
}

void UMatchmakingSubsystem::RemoveFromQueue(APlayerController* Player)
{
	Queue.Remove(Player);
}

void UMatchmakingSubsystem::ProcessMatchmaking()
{
	if (Queue.Num() < TargetTeamSize * 2) return; 
	
	if (!CurrentStrategy.GetInterface()) return;
	
	TArray<FMatchResult> Matches = CurrentStrategy->Execute_FindMatches(CurrentStrategy.GetObject(),Queue,TargetTeamSize);
	
	for (const FMatchResult& Match : Matches)
	{
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
		for (APlayerController* Player : Match.TeamA) Queue.Remove(Player);
		for (APlayerController* Player : Match.TeamB) Queue.Remove(Player);
	}
}

void UMatchmakingSubsystem::SetMatchmakingStrategy(TScriptInterface<IMatchmakingStrategy> NewStrategy)
{
	CurrentStrategy = NewStrategy;
}
