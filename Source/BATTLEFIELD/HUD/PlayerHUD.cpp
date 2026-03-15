// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUD.h"
#include "Announcement.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "ElimAnnouncement.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"

void APlayerHUD::DrawHUD()
{
	Super::DrawHUD();

	// 获取视口尺寸
	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		// 根据扩散值计算实际像素偏移
		float SpreadScaled = HUDPackage.CrosshairSpread * CrosshairSpreadMax;

		//============================
		// 绘制准星各部分
		//============================
		
		// 中心准星（无偏移）
		if (HUDPackage.CrosshairsCenter)
		{
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		// 左侧准星（向左偏移）
		if (HUDPackage.CrosshairsLeft)
		{
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		// 右侧准星（向右偏移）
		if (HUDPackage.CrosshairsRight)
		{
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		// 顶部准星（向上偏移）
		if (HUDPackage.CrosshairsTop)
		{
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
		// 底部准星（向下偏移）
		if (HUDPackage.CrosshairsButtom)
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsButtom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
	}
}

void APlayerHUD::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerHUD::AddCharacterOverlay()
{
	// 创建并添加角色状态面板到视口
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void APlayerHUD::AddAnnouncement()
{
	// 创建并添加公告面板到视口
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void APlayerHUD::AddElimAnnouncement(FString Attacker, FString Victim)
{
	// 确保拥有玩家控制器
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	
	if (OwningPlayer && ElimAnnouncementClass)
	{
		// 创建击杀公告Widget
		UElimAnnouncement* ElimAnnouncementWidget = CreateWidget<
			UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass);
		
		if (ElimAnnouncementWidget)
		{
			// 设置击杀文本
			ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			ElimAnnouncementWidget->AddToViewport();

			//============================
			// 移动旧消息向上
			//============================
			for (auto Msg : ElimMessages)
			{
				if (Msg && Msg->AnnouncementBox)
				{
					// 获取当前位置并向上移动
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementBox);
					if (CanvasSlot)
					{
						FVector2D Position = CanvasSlot->GetPosition();
						FVector2D NewPosition(Position.X, Position.Y - CanvasSlot->GetSize().Y);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			// 添加到消息列表
			ElimMessages.Add(ElimAnnouncementWidget);

			// 设置定时器，到期后移除该消息
			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget);

			GetWorldTimerManager().SetTimer(
				ElimMsgTimer,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false
			);
		}
	}
}

void APlayerHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread,
                               FLinearColor CrosshairColor)
{
	// 计算纹理尺寸
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	
	// 计算绘制位置（居中 + 偏移）
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	// 绘制纹理
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.f, 0.f, 1.f, 1.f,
	            CrosshairColor);
}

void APlayerHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	// 从父级移除公告Widget
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
	}
}
