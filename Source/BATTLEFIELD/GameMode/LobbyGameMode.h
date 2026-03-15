// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * @brief 大厅游戏模式 (LobbyGameMode)
 * 
 * 用于游戏开始前的等待大厅：
 * - 监听玩家加入
 * - 当玩家数达到设定值时自动切换到游戏地图
 * - 支持多种匹配类型（混战、团队、夺旗）
 * - 使用无缝旅行确保平滑过渡
 */
UCLASS()
class BATTLEFIELD_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	/**
	 * @brief 玩家登录后的处理
	 * @param NewPlayer 新登录的玩家控制器
	 * 
	 * 检查当前玩家数是否达到设定值，
	 * 如达到则根据匹配类型切换到相应地图
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
