// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

class UTextBlock;

/**
 * @brief 公告面板 (Announcement)
 * 
 * 显示游戏阶段公告信息：
 * - 热身倒计时
 * - 公告标题（如"等待玩家"、"比赛开始"）
 * - 信息文本（操作提示等）
 * 
 * 通常在匹配开始前显示，用于提示玩家当前状态
 */
UCLASS()
class BATTLEFIELD_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 热身倒计时文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarmupTime;

	/** 公告标题文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;

	/** 信息提示文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoText;
};
