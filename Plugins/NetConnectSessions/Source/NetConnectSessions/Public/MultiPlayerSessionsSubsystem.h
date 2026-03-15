/**
 * @file MultiPlayerSessionsSubsystem.h
 * @brief 多人游戏会话子系统头文件
 * @details 提供多人游戏会话的创建、查找、加入、销毁等功能，基于UE5的OnlineSubsystem
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiPlayerSessionsSubsystem.generated.h"

/** @brief 创建会话完成委托 - 动态多播，支持蓝图 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnCreateSessionComplete,bool,bWasSuccessful);
/** @brief 查找会话完成委托 - 多播 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiPlayerOnFindSessionsComplete,const TArray<FOnlineSessionSearchResult>& SessionResults,bool bWasSuccessful);
/** @brief 加入会话完成委托 - 多播 */
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnJoinSessionComplete,EOnJoinSessionCompleteResult::Type Result);
/** @brief 销毁会话完成委托 - 动态多播，支持蓝图 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnDestroySessionComplete,bool, bWasSuccessful);
/** @brief 启动会话完成委托 - 动态多播，支持蓝图 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnStartSessionComplete,bool, bWasSuccessful);

/**
 * @class UMultiPlayerSessionsSubsystem
 * @brief 多人游戏会话子系统
 * @details 管理游戏会话的生命周期，包括创建、查找、加入和销毁会话
 *          继承自UGameInstanceSubsystem，自动随GameInstance创建
 */
UCLASS()
class NETCONNECTSESSIONS_API UMultiPlayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** @brief 构造函数 - 初始化会话委托 */
	UMultiPlayerSessionsSubsystem();

	/**
	 * @name 会话操作
	 * @{
	 */
	
	/**
	 * @brief 创建新的游戏会话
	 * @param NumPublicConnections 公共连接数（最大玩家数）
	 * @param MatchType 匹配类型标识
	 */
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	
	/**
	 * @brief 查找可用的游戏会话
	 * @param MaxSearchResults 最大搜索结果数量
	 */
	void FindSessions(int32 MaxSearchResults);
	
	/**
	 * @brief 加入指定的游戏会话
	 * @param SessionResult 会话搜索结果
	 */
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	
	/** @brief 销毁当前游戏会话 */
	void DestroySession();
	
	/** @brief 启动游戏会话 */
	void StartSession();
	
	/** @} */

	/**
	 * @name 操作完成回调
	 * @{
	 */
	FMultiPlayerOnCreateSessionComplete MultiPlayerOnCreateSessionComplete;   /**< 创建会话完成回调 */
	FMultiPlayerOnFindSessionsComplete MultiPlayerOnFindSessionsComplete;     /**< 查找会话完成回调 */
	FMultiPlayerOnJoinSessionComplete MultiPlayerOnJoinSessionComplete;       /**< 加入会话完成回调 */
	FMultiPlayerOnDestroySessionComplete MultiPlayerOnDestroySessionComplete; /**< 销毁会话完成回调 */
	FMultiPlayerOnStartSessionComplete MultiPlayerOnStartSessionComplete;     /**< 启动会话完成回调 */
	/** @} */

protected:
	/**
	 * @name 内部回调函数 - 响应OnlineSubsystem委托
	 * @{
	 */
	void OnCreateSessionComplete(FName SessionName,bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	/** @} */

private:
	/** @brief 在线会话接口指针 */
	IOnlineSessionPtr SessionInterface;
	
	/** @brief 上次会话设置，用于创建会话 */
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	
	/** @brief 上次会话搜索，用于查找会话 */
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	/**
	 * @name 会话委托及句柄
	 * @details 用于向OnlineSubsystem注册回调并后续移除
	 * @{
	 */
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;
	/** @} */

	/** @brief 销毁后是否需要重新创建会话的标志 */
	bool bCreateSessionOnDestroy{ false };
	
	/** @brief 上次创建会话的连接数（用于销毁后重建） */
	int32 LastNumPublicConnections;
	
	/** @brief 上次创建会话的匹配类型（用于销毁后重建） */
	FString LastMatchType;
	
	/** @brief 期望的公共连接数 */
	int32 DesiredNumPublicConnections{};
	
	/** @brief 期望的匹配类型 */
	FString DesiredMatchType{};

public:
	/** @brief 获取期望的公共连接数 */
	int32 GetDesiredNumPublicConnections() const { return DesiredNumPublicConnections; }
	
	/** @brief 获取期望的匹配类型 */
	FString GetDesiredMatchType() const { return DesiredMatchType; }
};
