// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiPlayerSessionsSubsystem.generated.h"

/**
 * 
 */
 //�Զ���ί�����ڲ˵����а󶨻ص�
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnCreateSessionComplete,bool,bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiPlayerOnFindSessionsComplete,const TArray<FOnlineSessionSearchResult>& SessionResults,bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnJoinSessionComplete,EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnDestroySessionComplete,bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiPlayerOnStartSessionComplete,bool, bWasSuccessful);

UCLASS()
class NETCONNECTSESSIONS_API UMultiPlayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UMultiPlayerSessionsSubsystem();

	//�˵��ദ��Ự����
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	//Ϊ�˵��๫����ί��
	FMultiPlayerOnCreateSessionComplete MultiPlayerOnCreateSessionComplete;
	FMultiPlayerOnFindSessionsComplete MultiPlayerOnFindSessionsComplete;
	FMultiPlayerOnJoinSessionComplete MultiPlayerOnJoinSessionComplete;
	FMultiPlayerOnDestroySessionComplete MultiPlayerOnDestroySessionComplete;
	FMultiPlayerOnStartSessionComplete MultiPlayerOnStartSessionComplete;

protected:
	//�ڲ�ί�лص�������ӵ����߻Ự�ӿڴ����б�
	void OnCreateSessionComplete(FName SessionName,bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;


	//�󶨻ص��Ự���ܵ�ί��,��ӵ����߻Ự�ӿڴ����б�
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

	bool bCreateSessionOnDestroy{ false };
	int32 LastNumPublicConnections;
	FString LastMatchType;
	
	int32 DesiredNumPublicConnections{};
	FString DesiredMatchType{};
	
public:
	int32 GetDesiredNumPublicConnections() const { return DesiredNumPublicConnections; }
	FString GetDesiredMatchType() const { return DesiredMatchType; }
};
