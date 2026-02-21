
#pragma once

#include "CoreMinimal.h"
#include "MatchmakingStrategy.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchmakingSubsystem.generated.h"


UCLASS()
class MMRMATCHSYSTEM_API UMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void AddToQueue(APlayerController* Player,const FMatchmakingPreferences& Preferences);
	
	void RemoveFromQueue(APlayerController* Player);
	
	void ProcessMatchmaking();
	
	void SetMatchmakingStrategy(TScriptInterface<IMatchmakingStrategy> NewStrategy);

protected:
	//匹配间隔时间
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	float MatchmakingInterval = 2.f;
	
	//双方玩家数量
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	int32 TargetTeamSize = 5;
	
	//匹配队列
	UPROPERTY()
	TMap<APlayerController*,FMatchmakingPlayerData> Queue;
	
	//默认对局策略
	UPROPERTY()
	TScriptInterface<IMatchmakingStrategy> CurrentStrategy;
	
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	TSubclassOf<UObject> DefaultStrategyClass;
	
private:
	//定时器句柄
	FTimerHandle MatchmakingTimerHandle;
};
