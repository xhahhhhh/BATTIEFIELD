// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadWidget.h"
#include "Components/TextBlock.h"

void UOverHeadWidget::SetDisplayText(FString TextToDisplay)
{
	// 设置显示文本
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	// 获取Pawn的远程网络角色
	ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;

	// 转换角色枚举为可读字符串
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}

	// 格式化并显示
	FString RemoteRoleString = FString::Printf(TEXT("Remote Role:%s"), *Role);
	SetDisplayText(RemoteRoleString);
}

void UOverHeadWidget::NativeDestruct()
{
	// 从父级移除，确保正确清理
	RemoveFromParent();
	Super::NativeDestruct();
}
