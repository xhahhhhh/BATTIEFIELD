
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ElimAnnouncement.generated.h"

class UHorizontalBox;
class UTextBlock;

/**
 * @brief 击杀公告 (ElimAnnouncement)
 * 
 * 显示单个击杀信息的Widget，用于击杀滚动列表：
 * - 显示格式："攻击者 Killed 受害者"
 * - 包含在水平布局框中，便于定位和移动
 * 
 * 由PlayerHUD管理多个实例，形成滚动击杀列表
 */
UCLASS()
class BATTLEFIELD_API UElimAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置击杀公告文本
	 * @param AttackerName 攻击者名称
	 * @param VictimName 受害者名称
	 * 
	 * 格式化显示："AttackerName Killed VictimName"
	 */
	void SetElimAnnouncementText(FString AttackerName, FString VictimName);

public:
	/** 水平布局框（用于定位和移动整个公告） */
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* AnnouncementBox;

	/** 显示击杀信息的文本块 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;
};
