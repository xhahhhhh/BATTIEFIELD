

/**
 * @file Menu.cpp
 * @brief 多人游戏主菜单UI类实现文件
 * @details 实现游戏大厅UI的交互逻辑，包括创建/加入会话、游戏模式选择等功能
 */

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

/**
 * @brief 设置并初始化菜单
 * @param NumberofPublicConnections 公共连接数（最大玩家数）
 * @param TypeOfMatch 匹配类型
 * @param LobbyPath 大厅地图路径
 * @details 
 * - 设置UI显示和输入模式
 * - 获取多人会话子系统
 * - 绑定会话操作回调
 */
void UMenu::MenuSetup(int32 NumberofPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	/** @brief 构建大厅地图路径，添加listen参数 */
	PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	
	/** @brief 将UI添加到视口并设置可见性 */
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	/** @brief 配置输入模式为仅UI，显示鼠标光标 */
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

	/** @brief 从GameInstance获取多人会话子系统 */
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
	}

	/** @brief 绑定会话操作完成回调 */
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiPlayerOnCreateSessionComplete.AddDynamic(this, &UMenu::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiPlayerOnFindSessionsComplete.AddUObject(this, &UMenu::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiPlayerOnJoinSessionComplete.AddUObject(this, &UMenu::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.AddDynamic(this, &UMenu::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiPlayerOnStartSessionComplete.AddDynamic(this, &UMenu::OnStartSession);
	}
}


/**
 * @brief 初始化UI组件绑定
 * @return 初始化是否成功
 * @details 绑定所有UI控件的回调函数
 */
bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	/** @brief 绑定按钮点击事件 */
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
	
	/** @brief 绑定游戏模式复选框事件 */
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
	
	/** @brief 绑定玩家数量输入框事件 */
	if (NumPlayersTextBox)
	{
		NumPlayersTextBox->OnTextChanged.AddDynamic(this,&UMenu::NumPlayersTextBoxWrited);
	}

	return true;
}

/**
 * @brief 原生析构函数
 * @details UI销毁前清理菜单
 */
void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

/**
 * @brief 创建会话完成回调
 * @param bWasSuccessful 是否创建成功
 * @details 成功则跳转到大厅地图，失败则重新启用创建按钮
 */
void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		/** @brief 显示创建成功调试信息 */
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
		
		/** @brief 服务器跳转到大厅地图 */
		UWorld* World = GetWorld();
		FString TravelURL = PathToLobby + TEXT("?bShouldSeamlesslyTravel=false");
		if (World)
		{
			World->ServerTravel(TravelURL);
		}
	}
	else
	{
		/** @brief 显示创建失败调试信息 */
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				TEXT("Session Create Failed")
			);
		}
		/** @brief 重新启用创建按钮允许重试 */
		HostButton->SetIsEnabled(true);
	}
}

/**
 * @brief 查找会话完成回调
 * @param SessionResults 搜索结果数组
 * @param bWasSuccessful 是否查找成功
 * @details 遍历搜索结果，找到匹配类型相同的会话并加入
 */
void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr) return;

	/** @brief 遍历搜索结果，查找匹配类型相同的会话 */
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
	
	/** @brief 查找失败或无结果，重新启用加入按钮 */
	if (!bWasSuccessful || SessionResults.Num()==0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

/**
 * @brief 加入会话完成回调
 * @param Result 加入结果类型
 * @details 获取会话地址并执行客户端跳转
 */
void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	
	/** @brief 显示加入结果调试信息 */
	if (GEngine)
	{
		FString Message = FString::Printf(TEXT("JoinSession Result : %d"),Result);
		GEngine->AddOnScreenDebugMessage(-1,
				15.f,
				FColor::Yellow,
				Message
				);
	}
	
	/** @brief 获取会话连接地址并跳转 */
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
	
	/** @brief 加入失败则重新启用加入按钮 */
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		JoinButton->SetIsEnabled(true);
	}
}

/**
 * @brief 销毁会话完成回调
 * @param bWasSuccessful 是否销毁成功
 */
void UMenu::OnDestroySession(bool bWasSuccessful)
{
	
}

/**
 * @brief 启动会话完成回调
 * @param bWasSuccessful 是否启动成功
 */
void UMenu::OnStartSession(bool bWasSuccessful)
{
}

/**
 * @brief 创建主机按钮点击回调
 * @details 禁用按钮并调用子系统创建会话
 */
void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
}

/**
 * @brief 加入游戏按钮点击回调
 * @details 禁用按钮并调用子系统查找会话
 */
void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

/**
 * @brief 退出游戏按钮点击回调
 */
void UMenu::QuitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(
		this,
		GetWorld()->GetFirstPlayerController(),
		EQuitPreference::Quit,
		false
	);
}

/**
 * @brief 自由混战模式复选框点击回调
 * @param bIsChecked 是否被选中
 * @details 选中时取消其他模式选择，设置匹配类型为FreeForAll
 */
void UMenu::FreeForAllCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		TeamsCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		CaptureTheFlagCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("FreeForAll"));
	}
}

/**
 * @brief 团队模式复选框点击回调
 * @param bIsChecked 是否被选中
 * @details 选中时取消其他模式选择，设置匹配类型为Teams
 */
void UMenu::TeamsCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		FreeForAllCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		CaptureTheFlagCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("Teams"));
	}
}

/**
 * @brief 夺旗模式复选框点击回调
 * @param bIsChecked 是否被选中
 * @details 选中时取消其他模式选择，设置匹配类型为CaptureTheFlag
 */
void UMenu::CaptureTheFlagCheckBoxClicked(bool bIsChecked)
{
	if (bIsChecked)
	{
		FreeForAllCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		TeamsCheckBox->SetCheckedState(ECheckBoxState::Unchecked);
		MatchType = FString(TEXT("CaptureTheFlag"));
	}
}

/**
 * @brief 玩家数量输入框文字变化回调
 * @param Text 输入的文本
 * @details 将输入的文本转换为整数保存
 */
void UMenu::NumPlayersTextBoxWrited(const FText& Text)
{
	NumPublicConnections = FCString::Atoi(*Text.ToString());
}

/**
 * @brief 清理菜单UI
 * @details 从父节点移除UI，恢复游戏输入模式，隐藏鼠标
 */
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
