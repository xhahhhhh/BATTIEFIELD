// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenu.h"

#include "MultiPlayerSessionsSubsystem.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

bool UPauseMenu::Initialize()
{
	// 调用父类初始化
	if (!Super::Initialize())
	{
		return false;
	}
	return true;
}

void UPauseMenu::MenuSetup()
{
	//============================
	// 显示菜单
	//============================
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	//============================
	// 设置输入模式
	//============================
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			// 游戏+UI输入模式，允许同时操作游戏和Widget
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	//============================
	// 绑定按钮事件
	//============================
	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}

	//============================
	// 获取多人会话子系统
	//============================
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiPlayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
		if (MultiPlayerSessionsSubsystem)
		{
			// 绑定销毁会话完成回调
			MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.AddDynamic(
				this, &UPauseMenu::OnDetroySession);
		}
	}
}

void UPauseMenu::MenuTearDown()
{
	//============================
	// 隐藏菜单
	//============================
	RemoveFromParent();

	//============================
	// 恢复输入模式
	//============================
	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			// 纯游戏输入模式
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}

	//============================
	// 解绑事件
	//============================
	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UPauseMenu::ReturnButtonClicked);
	}
	if (MultiPlayerSessionsSubsystem && MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.IsBound())
	{
		MultiPlayerSessionsSubsystem->MultiPlayerOnDestroySessionComplete.RemoveDynamic(
			this, &UPauseMenu::OnDetroySession);
	}
}

void UPauseMenu::OnDetroySession(bool bWasSuccessful)
{
	// 销毁会话失败，重新启用按钮
	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	// 销毁会话成功，返回主菜单
	UWorld* World = GetWorld();
	if (World)
	{
		// 尝试获取游戏模式
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode)
		{
			// 主机：使用GameMode返回主菜单
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			// 客户端：通过PlayerController返回主菜单
			PlayerController = PlayerController == nullptr ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason(FText());
			}
		}
	}
}

void UPauseMenu::ReturnButtonClicked()
{
	// 禁用按钮防止重复点击
	ReturnButton->SetIsEnabled(false);

	UWorld* World = GetWorld();
	if (World)
	{
		// 获取玩家控制器和角色
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(FirstPlayerController->GetPawn());
			if (BaseCharacter)
			{
				// 绑定离开游戏回调
				BaseCharacter->OnLeftGame.AddDynamic(this, &UPauseMenu::OnPlayerLeftGame);
				// 通知服务器玩家要离开游戏
				BaseCharacter->ServerLeaveGame();
			}
			else
			{
				// 角色无效，重新启用按钮
				ReturnButton->SetIsEnabled(true);
			}
		}
	}
}

void UPauseMenu::OnPlayerLeftGame()
{
	// 玩家成功离开游戏，销毁会话
	if (MultiPlayerSessionsSubsystem)
	{
		MultiPlayerSessionsSubsystem->DestroySession();
	}
}
