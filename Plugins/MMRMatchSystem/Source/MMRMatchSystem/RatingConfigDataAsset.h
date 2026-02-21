#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RatingConfigDataAsset.generated.h"

//ELO参数配置
USTRUCT(BlueprintType)
struct FELOConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float BaseKFactor = 32.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float NewPlayerKFactor = 40.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 NewPlayerThreshold = 50;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float HighLevelThreshold = 1000;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float HighLevelKFactor = 16.f;
};

//MMR参数配置
USTRUCT(BlueprintType)
struct FMMRConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float BaseKFactor = 32.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float InitialUncertainty = 400.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float UncertaintyDecay = .98f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MinUncertainty = 50.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PerformanceImpact = 0.2f; 
};

//段位
USTRUCT(BlueprintType)
struct FRankTier
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString Name;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MinRating;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MaxRating;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FLinearColor Color;
};

UCLASS(BlueprintType)
class MMRMATCHSYSTEM_API URatingConfigDataAsset: public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="General")
	float DefaultRating = 1500.f;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="ELO")
	FELOConfig ELOConfig;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="MMR")
	FMMRConfig MMRConfig;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Ranks")
	TArray<FRankTier> RankTiers;
	
	UFUNCTION(BlueprintPure)
	FRankTier GetTierForRating(float Rating)const;
};
