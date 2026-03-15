#pragma once

/**
 * @file MatchmakingSubsystem.h
 * @brief 匹配子系统
 * 
 * 游戏实例级别的子系统，管理全局匹配队列：
 * - 玩家加入/离开匹配队列
 * - 定时执行匹配算法
 * - 支持可插拔的匹配策略
 */

#include "CoreMinimal.h"
#include "MatchmakingStrategy.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchmakingSubsystem.generated.h"

/**
 * @brief 匹配子系统类
 * 
 * 继承自UGameInstanceSubsystem，生命周期与游戏实例绑定
 * 负责管理匹配队列和调用匹配策略
 */
UCLASS()
class MMRMATCHSYSTEM_API UMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** 子系统初始化 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/** 子系统反初始化 */
	virtual void Deinitialize() override;
	
	/**
	 * @brief 将玩家加入匹配队列
	 * @param Player 玩家控制器
	 * @param Preferences 匹配偏好（角色偏好等）
	 */
	void AddToQueue(APlayerController* Player, const FMatchmakingPreferences& Preferences);
	
	/**
	 * @brief 将玩家移出匹配队列
	 * @param Player 玩家控制器
	 */
	void RemoveFromQueue(APlayerController* Player);
	
	/**
	 * @brief 执行匹配逻辑
	 * 
	 * 由定时器定期调用，根据当前策略寻找匹配
	 */
	void ProcessMatchmaking();
	
	/**
	 * @brief 设置匹配策略
	 * @param NewStrategy 新的匹配策略接口
	 */
	void SetMatchmakingStrategy(TScriptInterface<IMatchmakingStrategy> NewStrategy);

protected:
	/** 匹配间隔时间（秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	float MatchmakingInterval = 2.f;
	
	/** 每队目标玩家数量 */
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	int32 TargetTeamSize = 5;
	
	/** 匹配队列（玩家控制器 -> 玩家数据） */
	UPROPERTY()
	TMap<APlayerController*, FMatchmakingPlayerData> Queue;
	
	/** 当前使用的匹配策略 */
	UPROPERTY()
	TScriptInterface<IMatchmakingStrategy> CurrentStrategy;
	
	/** 默认策略类 */
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	TSubclassOf<UObject> DefaultStrategyClass;
	
private:
	/** 匹配定时器句柄 */
	FTimerHandle MatchmakingTimerHandle;
};
