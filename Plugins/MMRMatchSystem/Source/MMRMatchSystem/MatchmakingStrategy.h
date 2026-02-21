
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MatchmakingStrategy.generated.h"

//对局偏好，如常用角色、常选位置
USTRUCT(BlueprintType)
struct FMatchmakingPreferences
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> PreferredRoles;

};

//存储玩家对局数据
USTRUCT(BlueprintType)
struct FMatchmakingPlayerData
{
	GENERATED_BODY()
	
	UPROPERTY()
	APlayerController* Player = nullptr;
	
	UPROPERTY()
	float Rating = 1500.f;
	
	UPROPERTY()
	float Uncertainty = 400.f;
	
	UPROPERTY()
	float WaitTime = 0.f;
	
	UPROPERTY()
	TArray<FString> PreferredRoles;
	
	UPROPERTY()
	int32 WinStreak = 0;
	
	bool operator==(const FMatchmakingPlayerData& Other) const
	{
		return Player == Other.Player;
	}
};

//匹配对局结果，双方玩家以及对局平均分
USTRUCT(BlueprintType)
struct FMatchResult
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<APlayerController*> TeamA;
	
	UPROPERTY()
	TArray<APlayerController*> TeamB;
	
	UPROPERTY()
	float BalanceScore = 0.f;
};

UINTERFACE()
class UMatchmakingStrategy : public UInterface
{
	GENERATED_BODY()
};


class MMRMATCHSYSTEM_API IMatchmakingStrategy
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent)
	TArray<FMatchResult> FindMatches(const TMap<APlayerController*, FMatchmakingPlayerData>& Queue,int32 TeamSize);
	
	UFUNCTION(BlueprintNativeEvent)
	float EvaluateMatchQuality(const FMatchResult& Match)const;
};

UCLASS(BlueprintType)
class UDefaultMatchmakingStrategy : public UObject,public IMatchmakingStrategy
{
	GENERATED_BODY()

public:
	virtual TArray<FMatchResult> FindMatches_Implementation(const TMap<APlayerController*, FMatchmakingPlayerData>& Queue,int32 TeamSize)override;
	virtual float EvaluateMatchQuality_Implementation(const FMatchResult& Match)const override;
	
	UPROPERTY(EditDefaultsOnly,Category="Config")
	float TargetBalanceThreshold = .7f;
	
	UPROPERTY(EditDefaultsOnly,Category="Config")
	float MaxWaitTimeBeforeCompromise = 1800.0f;
	
	UPROPERTY(EditDefaultsOnly,Category="Config")
	bool bEnableRoleMatching = true;
};
