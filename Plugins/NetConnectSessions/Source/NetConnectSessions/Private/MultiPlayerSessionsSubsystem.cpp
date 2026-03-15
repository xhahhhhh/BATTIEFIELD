/**
 * @file MultiPlayerSessionsSubsystem.cpp
 * @brief 多人游戏会话子系统实现文件
 * @details 实现会话的创建、查找、加入、销毁等核心功能
 */

#include "MultiPlayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"

/**
 * @brief 构造函数
 * @details 初始化所有会话委托，并获取在线子系统的会话接口
 */
UMultiPlayerSessionsSubsystem::UMultiPlayerSessionsSubsystem() :
	CreateSessionCompleteDelegate(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	/** @brief 获取全局在线子系统实例 */
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (Subsystem)
	{
		/** @brief 从子系统获取会话管理接口 */
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

/**
 * @brief 创建新的游戏会话
 * @param NumPublicConnections 最大公共连接数（玩家数）
 * @param MatchType 游戏匹配类型标识
 * @details 
 * - 检查是否已存在会话，存在则先销毁后重建
 * - 配置会话设置（LAN模式、连接数、广播等）
 * - 通过OnlineSubsystem创建会话
 */
void UMultiPlayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	/** @brief 保存期望的会话参数 */
	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;
	
	if (!SessionInterface.IsValid()) return;

	/** @brief 检查是否已存在同名会话 */
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		/** @brief 标记需要销毁后重建，保存参数 */
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		DestroySession();
		return;
	}

	/** @brief 向会话接口注册创建完成委托 */
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		CreateSessionCompleteDelegate);

	/** @brief 创建并配置会话设置 */
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	
	/** @brief 根据子系统类型决定是否为LAN模式（NULL子系统使用LAN） */
	LastSessionSettings->bIsLANMatch = Subsystem->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;  // 允许游戏进行中加入
	LastSessionSettings->bAllowJoinViaPresence = true; // 允许通过Presence加入
	LastSessionSettings->bShouldAdvertise = true;      // 广播会话让其他玩家发现
	LastSessionSettings->bUsesPresence = true;         // 使用Presence功能
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;
	LastSessionSettings->bUseLobbiesIfAvailable = true; // 优先使用大厅系统

	/** @brief 获取本地玩家并创建会话 */
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession,
	                                     *LastSessionSettings))
	{
		/** @brief 创建失败，清理委托并广播失败 */
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		MultiPlayerOnCreateSessionComplete.Broadcast(false);
	}
}

/**
 * @brief 查找可用的游戏会话
 * @param MaxSearchResults 最大搜索结果数量
 * @details 搜索局域网或在线会话，结果通过委托广播
 */
void UMultiPlayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid()) return;
	
	/** @brief 注册查找完成委托 */
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FindSessionsCompleteDelegate);

	/** @brief 创建搜索配置 */
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	
	/** @brief 根据子系统决定是否为LAN搜索 */
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	LastSessionSearch->bIsLanQuery = Subsystem->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	
	/** @brief 执行搜索 */
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		/** @brief 搜索失败，清理委托并广播空结果 */
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		MultiPlayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

/**
 * @brief 加入指定的游戏会话
 * @param SessionResult 要加入的会话搜索结果
 * @details 尝试加入指定的会话，结果通过委托广播
 */
void UMultiPlayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	/** @brief 检查会话接口有效性 */
	if (!SessionInterface.IsValid())
	{
		MultiPlayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	/** @brief 注册加入完成委托 */
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	
	/** @brief 执行加入操作 */
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		/** @brief 加入失败，清理委托并广播错误 */
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		MultiPlayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

/**
 * @brief 销毁当前游戏会话
 * @details 销毁名为NAME_GameSession的会话，结果通过委托广播
 */
void UMultiPlayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiPlayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	/** @brief 注册销毁完成委托 */
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		DestroySessionCompleteDelegate);
	
	/** @brief 执行销毁操作 */
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		/** @brief 销毁失败，清理委托并广播失败 */
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiPlayerOnDestroySessionComplete.Broadcast(false);
	}
}

/**
 * @brief 启动游戏会话
 * @details 当前为空实现
 */
void UMultiPlayerSessionsSubsystem::StartSession()
{
}

/**
 * @brief 创建会话完成回调
 * @param SessionName 会话名称
 * @param bWasSuccessful 是否成功
 * @details 清理委托并广播结果
 */
void UMultiPlayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	MultiPlayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

/**
 * @brief 查找会话完成回调
 * @param bWasSuccessful 是否成功
 * @details 清理委托并广播搜索结果
 */
void UMultiPlayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	
	/** @brief 无搜索结果也视为失败 */
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiPlayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	MultiPlayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

/**
 * @brief 加入会话完成回调
 * @param SessionName 会话名称
 * @param Result 加入结果类型
 * @details 清理委托并广播结果
 */
void UMultiPlayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	MultiPlayerOnJoinSessionComplete.Broadcast(Result);
}

/**
 * @brief 销毁会话完成回调
 * @param SessionName 会话名称
 * @param bWasSuccessful 是否成功
 * @details 清理委托，如设置了重建标志则重新创建会话
 */
void UMultiPlayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	
	/** @brief 销毁成功后如需要则重建会话 */
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	MultiPlayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

/**
 * @brief 启动会话完成回调
 * @param SessionName 会话名称
 * @param bWasSuccessful 是否成功
 */
void UMultiPlayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
