
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

class UButton;
class UMultiPlayerSessionsSubsystem;
UCLASS()
class NETCONNECTSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberofPublicConnections =4,FString TypeOfMatch = FString(TEXT("FreeForAll")),FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	//Ϊ���˻Ự��ϵͳ���õĻص�����
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	UFUNCTION()
	void QuitButtonClicked();

	void MenuTearDown();

	//多人会话子系统
	UMultiPlayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	int32 NumPublicConnections{4};
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	FString MatchType{TEXT("FreeForAll")};
	
	FString PathToLobby{ TEXT("") };
};
