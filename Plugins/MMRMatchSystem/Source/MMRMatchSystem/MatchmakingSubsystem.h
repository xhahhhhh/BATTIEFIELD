
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
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	float MatchmakingInterval = 2.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	int32 TargetTeamSize = 5;
	
	UPROPERTY()
	TMap<APlayerController*,FMatchmakingPlayerData> Queue;
	
	UPROPERTY()
	TScriptInterface<IMatchmakingStrategy> CurrentStrategy;
	
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	TSubclassOf<UObject> DefaultStrategyClass;
	
private:
	FTimerHandle MatchmakingTimerHandle;
};
