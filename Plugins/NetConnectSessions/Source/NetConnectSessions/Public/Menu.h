
/**
 * @file Menu.h
 * @brief 多人游戏主菜单UI类头文件
 * @details 提供主机、加入、退出游戏等功能，以及游戏模式选择和玩家数量设置
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

// 前向声明UI组件类
class UEditableTextBox;
class UCheckBox;
class UButton;
class UMultiPlayerSessionsSubsystem;

/**
 * @class UMenu
 * @brief 多人游戏主菜单UI类
 * @details 继承自UUserWidget，处理游戏大厅的UI交互和会话管理
 */
UCLASS()
class NETCONNECTSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * @brief 设置并初始化菜单
	 * @param NumberofPublicConnections 公共连接数（最大玩家数），默认4
	 * @param TypeOfMatch 匹配类型，默认"FreeForAll"
	 * @param LobbyPath 大厅地图路径
	 * @details 设置UI显示、输入模式，并绑定会话回调
	 */
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberofPublicConnections =4,FString TypeOfMatch = FString(TEXT("FreeForAll")),FString LobbyPath = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));

protected:
	/**
	 * @brief 初始化UI组件绑定
	 * @return 初始化是否成功
	 * @details 绑定按钮点击事件和复选框状态变化事件
	 */
	virtual bool Initialize() override;
	
	/**
	 * @brief 原生析构函数
	 * @details UI销毁时清理菜单，恢复输入模式
	 */
	virtual void NativeDestruct() override;

	/**
	 * @name 会话操作回调函数
	 * @{
	 */
	
	/** @brief 创建会话完成回调 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);

	/** @brief 查找会话完成回调 */
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	
	/** @brief 加入会话完成回调 */
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	
	/** @brief 销毁会话完成回调 */
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	/** @brief 启动会话完成回调 */
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);
	
	/** @} */

private:
	/**
	 * @name UI组件绑定（通过UMG编辑器绑定）
	 * @{
	 */
	
	/** @brief 创建主机按钮 */
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	/** @brief 加入游戏按钮 */
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	/** @brief 退出游戏按钮 */
	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;
	
	/** @brief 自由混战模式复选框 */
	UPROPERTY(meta = (BindWidget))
	UCheckBox* FreeForAllCheckBox;
	
	/** @brief 团队模式复选框 */
	UPROPERTY(meta = (BindWidget))
	UCheckBox* TeamsCheckBox;
	
	/** @brief 夺旗模式复选框 */
	UPROPERTY(meta = (BindWidget))
	UCheckBox* CaptureTheFlagCheckBox;
	
	/** @brief 玩家数量输入框 */
	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* NumPlayersTextBox;
	
	/** @} */

	/**
	 * @name UI按钮/控件回调函数
	 * @{
	 */
	
	/** @brief 创建主机按钮点击回调 */
	UFUNCTION()
	void HostButtonClicked();

	/** @brief 加入游戏按钮点击回调 */
	UFUNCTION()
	void JoinButtonClicked();

	/** @brief 退出游戏按钮点击回调 */
	UFUNCTION()
	void QuitButtonClicked();
	
	/** @brief 自由混战复选框状态变化回调 */
	UFUNCTION()
	void FreeForAllCheckBoxClicked(bool bIsChecked);
	
	/** @brief 团队模式复选框状态变化回调 */
	UFUNCTION()
	void TeamsCheckBoxClicked(bool bIsChecked);
	
	/** @brief 夺旗模式复选框状态变化回调 */
	UFUNCTION()
	void CaptureTheFlagCheckBoxClicked(bool bIsChecked);
	
	/** @brief 玩家数量输入变化回调 */
	UFUNCTION()
	void NumPlayersTextBoxWrited(const FText& Text);
	
	/** @} */

	/** @brief 清理菜单UI，恢复游戏输入模式 */
	void MenuTearDown();

	/** @brief 多人会话子系统指针 */
	UMultiPlayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	/** @brief 公共连接数（最大玩家数） */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	int32 NumPublicConnections{4};
	
	/** @brief 当前选择的匹配类型 */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	FString MatchType{TEXT("FreeForAll")};
	
	/** @brief 大厅地图路径 */
	FString PathToLobby{ TEXT("") };
};
