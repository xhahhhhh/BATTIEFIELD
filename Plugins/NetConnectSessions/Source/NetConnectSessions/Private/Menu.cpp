

#include "Menu.h"
#include "Components/Button.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"

void UMenu::MenuSetup(int32 NumberofPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	// NumPublicConnections = NumberofPublicConnections;
	// MatchType = TypeOfMatch;
	
	//启用UI并打印在屏幕上
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	//配置用户输入以及鼠标
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	//获取多人会话子系统并绑定回调
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiPlayerOnCreateSessionComplete.AddDynamic(this, &UMenu::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiPlayerOnFindSessionsComplete.AddUObject(this, &UMenu::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiPlayerOnJoinSessionComplete.AddUObject(this, &UMenu::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.AddDynamic(this, &UMenu::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiPlayerOnStartSessionComplete.AddDynamic(this, &UMenu::OnStartSession);
	}
}


bool UMenu::Initialize()
{
	//初始化绑定UI回调
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMenu::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMenu::JoinButtonClicked);
	}
	if (QuitButton)

	{
		QuitButton->OnClicked.AddDynamic(this, &UMenu::QuitButtonClicked);
	}
	if (FreeForAllCheckBox)
	{
		FreeForAllCheckBox->OnCheckStateChanged.AddDynamic(this,&UMenu::FreeForAllCheckBoxClicked);
	}
	if (TeamsCheckBox)
	{
		TeamsCheckBox->OnCheckStateChanged.AddDynamic(this,&UMenu::TeamsCheckBoxClicked);
	}
	if (CaptureTheFlagCheckBox)
	{
		CaptureTheFlagCheckBox->OnCheckStateChanged.AddDynamic(this,&UMenu::CaptureTheFlagCheckBoxClicked);
	}
	if (NumPlayersTextBox)
	{
		NumPlayersTextBox->OnTextChanged.AddDynamic(this,&UMenu::NumPlayersTextBoxWrited);
	}

	return true;;
}

void UMenu::NativeDestruct()
{
	//销毁
	MenuTearDown();
	Super::NativeDestruct();
}

//创建会话完成时回调
void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		FString Message = FString::Printf(TEXT("Session Created: %s,%d"),*MatchType,NumPublicConnections);
		if (GEngine)
		{
			
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				Message
			);
		}
		UWorld* World = GetWorld();
		FString TravelURL = PathToLobby + TEXT("?bShouldSeamlesslyTravel=false");
		if (World)
		{
			World->ServerTravel(TravelURL);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				TEXT("Session Create Failed")
			);
		}
		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)return;

	for (auto Result : SessionResults)
	{
		FString SettingsValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		if (SettingsValue == MatchType)
		{
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
	if (!bWasSuccessful || SessionResults.Num()==0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("JoinSession Result : %d"),Result);
		GEngine->AddOnScreenDebugMessage(-1,
				15.f,
				FColor::Yellow,
				Message
				);
	}
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			bool bGotAddress = SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
			FString Message = FString::Printf(TEXT("Travel Failed : %s"),*Address);
			GEngine->AddOnScreenDebugMessage(-1,
			15.f,
			FColor::Yellow,
			Message
			);
			if (bGotAddress)
			{
				APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
				if (PlayerController)
				{
					PlayerController->ClientTravel(Address, TRAVEL_Absolute);
				}
			}
		}
	}
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
	
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);

	}
}

void UMenu::QuitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(
		this,
		GetWorld()->GetFirstPlayerController(),
		EQuitPreference::Quit,
		false
	);
}

void UMenu::FreeForAllCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		TeamsCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		CaptureTheFlagCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("FreeForAll"));
	}
}

void UMenu::TeamsCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		FreeForAllCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		CaptureTheFlagCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("Teams"));
	}
}

void UMenu::CaptureTheFlagCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		FreeForAllCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		TeamsCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("CaptureTheFlag"));
	}
}

void UMenu::NumPlayersTextBoxWrited(const FText& Text)
{
	NumPublicConnections = FCString::Atoi(*Text.ToString());
}

//UI界面销毁并设置输入
void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
