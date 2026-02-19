
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RatingAlgorithm.h"
#include "PlayerRatingComponent.generated.h"

struct FPerformanceMetrics;

UENUM(BlueprintType)
enum class ERatingAlgorithmType:uint8
{
	ELO UMETA(DisplayName = "ELO"),
	MMR UMETA(DisplayName = "MMR"),
	
	MAX UMETA(DisplayName = "DefaultMax")
};

USTRUCT(BlueprintType)
struct FPlayerRatingData
{
	GENERATED_BODY()
	
	UPROPERTY(SaveGame)
	float HiddenRating = 1500.f;
	
	UPROPERTY(SaveGame)
	float Uncertainty = 400.f;
	
	UPROPERTY(SaveGame)
	int32 GamesPlayed = 0;
	
	UPROPERTY(SaveGame)
	int32 Wins = 0;
	
	UPROPERTY(SaveGame)
	TArray<float> RecentPerformances;
	
	UPROPERTY(SaveGame)
	TMap<FString, float> RoleProficiency;
	
	UPROPERTY(SaveGame)
	TArray<float> RatingHistory;
	
	//连胜数
	UPROPERTY(SaveGame)
	int32 WinStreak = 0;
	
	//连败数
	UPROPERTY(SaveGame)
	int32 LoseStreak = 0;
	
	UPROPERTY(SaveGame)
	FDateTime LastUpdateTime;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MMRMATCHSYSTEM_API UPlayerRatingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerRatingComponent();
	void ConfigAlgorithmInstance();

	virtual void BeginPlay() override;

	void SetRatingAlgorithm(ERatingAlgorithmType NewAlgorithmType);

	void UpdateRatingForTeamGame(
		float TeamAverageRating,
		float OpponentAverageRating,
		bool bWon,
		const FPerformanceMetrics& Performance,
		const FString& Role = TEXT("")
	);

	void UpdateRatingForDuel(float OpponentRating, bool bWon, float KFactor = -1.0f);
	float GetMatchmakingRating()const;
	FString GetVisibleRank() const;
	FLinearColor GetRankColor() const;
	
protected:
	UPROPERTY(BlueprintReadOnly,Category="Rating")
	ERatingAlgorithmType AlgorithmType = ERatingAlgorithmType::MMR;
	
	UPROPERTY(BlueprintReadOnly,Category="Rating")
	class URatingConfigDataAsset* RatingConfig;
	
	UPROPERTY(BlueprintReadOnly,Category="Rating",SaveGame)
	FPlayerRatingData RatingData;
	
private:
	TUniquePtr<IRatingAlgorithm> AlgorithmInstance;
	
	void UpdateStreak(bool bWon);
	
public:
	float GetUncertainty() const { return RatingData.Uncertainty; }
	int32 GetWinStreak() const { return RatingData.WinStreak; }
};
