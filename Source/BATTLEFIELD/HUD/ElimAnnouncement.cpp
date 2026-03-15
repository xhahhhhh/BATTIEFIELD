
/**
 * @file ElimAnnouncement.cpp
 * @brief 击杀公告实现
 * 
 * 实现单个击杀信息的显示功能。
 */

#include "ElimAnnouncement.h"

#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(FString AttackerName, FString VictimName)
{
	// 格式化击杀信息："攻击者 Killed 受害者"
	FString ElimAnnouncementText = FString::Printf(TEXT("%s Killed %s"), *AttackerName, *VictimName);
	
	// 设置到文本组件
	if (AnnouncementText)
	{
		AnnouncementText->SetText(FText::FromString(ElimAnnouncementText));
	}
}
