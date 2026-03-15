// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverHeadWidget.generated.h"

class UTextBlock;

/**
 * @brief 头顶显示Widget (OverHeadWidget)
 * 
 * 显示在角色头顶的3D UI元素，主要用于：
 * - 调试显示网络角色（Authority/Autonomous/Simulated Proxy）
 * - 可扩展：显示玩家名称、血量条等
 * 
 * 通常通过WidgetComponent附加到角色上
 */
UCLASS()
class BATTLEFIELD_API UOverHeadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置显示文本
	 * @param TextToDisplay 要显示的文本内容
	 */
	void SetDisplayText(FString TextToDisplay);

	/**
	 * @brief 显示玩家的网络角色
	 * @param InPawn 要显示网络角色的Pawn
	 * 
	 * 用于调试，显示该Pawn在本地机器上的网络模拟角色：
	 * - Authority: 服务器权威
	 * - Autonomous Proxy: 本地控制的玩家
	 * - Simulated Proxy: 其他玩家的角色（由服务器同步）
	 */
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

protected:
	/**
	 * @brief Widget销毁时调用
	 * 
	 * 从父级移除自己，确保正确清理
	 */
	virtual void NativeDestruct() override;

public:
	/** 显示文本的TextBlock组件 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DisplayText;
};
