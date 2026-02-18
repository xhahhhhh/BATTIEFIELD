#pragma once

#include "CoreMinimal.h"
#include "PerformanceMetrics.generated.h"

USTRUCT(BlueprintType)
struct FGameEvent
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EventType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Timestamp;
};

USTRUCT(BlueprintType)
struct FPerformanceMetrics
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KDA = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageDone = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealingDone = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ObjectiveScore = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SurvivalTime = 0.0f;
	
	float GetOverallPerformance() const
	{
		float Score = (KDA+DamageDone+HealingDone+ObjectiveScore+SurvivalTime)*0.3f;
		return FMath::Clamp(Score, 0.f, 1.f);
	}
};


