// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenu.generated.h"

class UButton;
class UMultiPlayerSessionsSubsystem;
class APlayerController;

/**
 * @brief 暂停菜单 (PauseMenu)
 * 
 * 游戏暂停时显示的菜单界面：
 * - 显示返回主菜单按钮
 * - 处理离开游戏的完整流程
 * - 销毁多人游戏会话
 * - 返回主菜单（主机或客户端分别处理）
 */
UCLASS()
class BATTLEFIELD_API UPauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置菜单显示
	 * 
	 * 添加到视口、设置输入模式（游戏+UI）、显示鼠标、绑定按钮事件
	 */
	void MenuSetup();

	/**
	 * @brief 关闭菜单
	 * 
	 * 从视口移除、恢复纯游戏输入模式、隐藏鼠标、解绑事件
	 */
	void MenuTearDown();

protected:
	/**
	 * @brief 初始化Widget
	 * @return 初始化是否成功
	 */
	virtual bool Initialize() override;

	/**
	 * @brief 销毁会话完成回调
	 * @param bWasSuccessful 是否成功销毁会话
	 * 
	 * 成功销毁后返回主菜单
	 */
	UFUNCTION()
	void OnDetroySession(bool bWasSuccessful);

	/**
	 * @brief 玩家离开游戏回调
	 * 
	 * 玩家成功离开游戏后销毁会话
	 */
	UFUNCTION()
	void OnPlayerLeftGame();

private:
	/**
	 * @brief 返回按钮点击处理
	 * 
	 * 禁用按钮、通知服务器玩家离开游戏
	 */
	UFUNCTION()
	void ReturnButtonClicked();

private:
	/** 返回主菜单按钮 */
	UPROPERTY(meta = (BindWidget))
	UButton* ReturnButton;

	/** 多人会话子系统引用 */
	UPROPERTY()
	UMultiPlayerSessionsSubsystem* MultiPlayerSessionsSubsystem;

	/** 玩家控制器引用 */
	UPROPERTY()
	APlayerController* PlayerController;
};
