
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "PerformanceMetrics.h"
#include "PerformanceEvaluator.generated.h"

USTRUCT(BlueprintType)
struct FRoleWeightConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, float> EventWeights;
};

UINTERFACE(BlueprintType)
class UPerformanceEvaluationStrategy : public UInterface
{
	GENERATED_BODY()
};

class MMRMATCHSYSTEM_API IPerformanceEvaluationStrategy
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent)
	float EvaluatePerformance(const TArray<FGameEvent>& PlayerEvents,const FString& Role) const;
};

UCLASS(BlueprintType)
class UPerformanceEvaluator:public UObject,public IPerformanceEvaluationStrategy
{
	GENERATED_BODY()

public:
	//根据比赛中的事件计算表现分
	virtual float EvaluatePerformance_Implementation(const TArray<FGameEvent>& PlayerEvents,const FString& Role) const override;
	
	//事件权重
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	FRoleWeightConfig TotalEventWeights;
	
	//角色表现权重
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TMap<FString, FRoleWeightConfig> RoleSpecificWeights;
};
