#pragma once

#include "CoreMinimal.h"
#include "PerformanceMetrics.generated.h"

//游戏事件，如连杀、超神、一血，用以改变表现分
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

//对局表现参数
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
	
	//计算对局表现并限制到0-1
	float GetOverallPerformance() const
	{
		float Score = (KDA+DamageDone+HealingDone+ObjectiveScore+SurvivalTime)*0.3f;
		return FMath::Clamp(Score, 0.f, 1.f);
	}
};


