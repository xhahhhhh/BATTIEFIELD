
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RatingAlgorithm.h"
#include "PlayerRatingComponent.generated.h"

struct FPerformanceMetrics;

//匹配机制枚举
UENUM(BlueprintType)
enum class ERatingAlgorithmType:uint8
{
	ELO UMETA(DisplayName = "ELO"),
	MMR UMETA(DisplayName = "MMR"),
	
	MAX UMETA(DisplayName = "DefaultMax")
};

//玩家数据
USTRUCT(BlueprintType)
struct FPlayerRatingData
{
	GENERATED_BODY()
	
	//隐藏分
	UPROPERTY(SaveGame)
	float HiddenRating = 1500.f;
	
	//不确定性默认为400
	UPROPERTY(SaveGame)
	float Uncertainty = 400.f;
	
	//总对局数
	UPROPERTY(SaveGame)
	int32 GamesPlayed = 0;
	
	//胜利数
	UPROPERTY(SaveGame)
	int32 Wins = 0;
	
	//最近表现分
	UPROPERTY(SaveGame)
	TArray<float> RecentPerformances;
	
	//角色熟练度
	UPROPERTY(SaveGame)
	TMap<FString, float> RoleProficiency;
	
	//历史记录
	UPROPERTY(SaveGame)
	TArray<float> RatingHistory;
	
	//连胜数
	UPROPERTY(SaveGame)
	int32 WinStreak = 0;
	
	//连败数
	UPROPERTY(SaveGame)
	int32 LoseStreak = 0;
	
	//最后更新时间
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
	//算法类型
	UPROPERTY(BlueprintReadOnly,Category="Rating")
	ERatingAlgorithmType AlgorithmType = ERatingAlgorithmType::MMR;
	
	//评分配置
	UPROPERTY(BlueprintReadOnly,Category="Rating")
	class URatingConfigDataAsset* RatingConfig;
	
	//玩家数据
	UPROPERTY(BlueprintReadOnly,Category="Rating",SaveGame)
	FPlayerRatingData RatingData;
	
private:
	//算法实例
	TUniquePtr<IRatingAlgorithm> AlgorithmInstance;
	
	void UpdateStreak(bool bWon);
	
public:
	float GetUncertainty() const { return RatingData.Uncertainty; }
	int32 GetWinStreak() const { return RatingData.WinStreak; }
};
