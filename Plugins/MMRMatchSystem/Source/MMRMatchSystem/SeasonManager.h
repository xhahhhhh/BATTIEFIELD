
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SeasonManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonEnded, int32, SeasonNumber);

USTRUCT(BlueprintType)
struct FSeasonData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 SeasonNumber = 1;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime StartDate;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime EndDate;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;
	
};

UCLASS(BlueprintType, Blueprintable)
class MMRMATCHSYSTEM_API USeasonManager : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(int32 Season,const FDateTime& Start,const FDateTime& End);
	
	UFUNCTION(BlueprintCallable)
	void UpdateSeason();
	
	UFUNCTION(BlueprintCallable)
	float ApplySoftReset(float CurrentRating,int32 GamesPlayed,float WinRate) const;
	
	UFUNCTION(BlueprintPure)
	FSeasonData GetSeasonData() const {return CurrentSeason;}
	
	UPROPERTY(BlueprintAssignable)
	FOnSeasonEnded OnSeasonEnded;
	
protected:
	UPROPERTY(BlueprintReadOnly,Category="Config")
	int32 SeasonDurationDays = 90;
	
	UPROPERTY(BlueprintReadOnly,Category="Config")
	float SoftResetStrength = .3f;
	
	UPROPERTY(BlueprintReadOnly,Category="Config")
	float DefaultRating = 1500.f;
	
private:
	FSeasonData CurrentSeason;
	
};
