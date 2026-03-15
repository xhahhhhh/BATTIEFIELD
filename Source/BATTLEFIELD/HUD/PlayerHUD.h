// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlayerHUD.generated.h"

class UUserWidget;
class UCharacterOverlay;
class UAnnouncement;
class UElimAnnouncement;

/**
 * @brief HUD包结构体
 * 
 * 封装准星显示所需的所有数据：
 * - 各方向准星纹理（中心、左、右、上、下）
 * - 准星扩散值（随移动/射击增大）
 * - 准星颜色
 */
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	/** 中心准星纹理 */
	class UTexture2D* CrosshairsCenter;
	/** 左侧准星纹理 */
	UTexture2D* CrosshairsLeft;
	/** 右侧准星纹理 */
	UTexture2D* CrosshairsRight;
	/** 顶部准星纹理 */
	UTexture2D* CrosshairsTop;
	/** 底部准星纹理（注意拼写：Buttom应为Bottom） */
	UTexture2D* CrosshairsButtom;
	/** 准星扩散值（0-1范围，影响准星展开程度） */
	float CrosshairSpread;
	/** 准星颜色 */
	FLinearColor CrosshairsColor;
};

/**
 * @brief 玩家HUD (PlayerHUD)
 * 
 * 管理所有玩家界面元素的显示：
 * - 准星绘制：动态扩散的十字准星
 * - 角色状态面板：血量、护盾、弹药、分数等
 * - 公告面板：热身倒计时、匹配信息
 * - 击杀公告：滚动显示的击杀信息
 */
UCLASS()
class BATTLEFIELD_API APlayerHUD : public AHUD
{
	GENERATED_BODY()

public:
	/**
	 * @brief 绘制HUD
	 * 
	 * 每帧调用，负责绘制准星
	 */
	virtual void DrawHUD() override;

	//============================
	// 角色状态面板
	//============================

	/** 角色状态面板Widget类 */
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	/** 创建并显示角色状态面板 */
	void AddCharacterOverlay();

	/** 角色状态面板实例 */
	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	//============================
	// 公告面板
	//============================

	/** 公告面板Widget类 */
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	/** 公告面板实例 */
	UPROPERTY()
	UAnnouncement* Announcement;

	/** 创建并显示公告面板 */
	void AddAnnouncement();

	/**
	 * @brief 添加击杀公告
	 * @param Attacker 攻击者名称
	 * @param Victim 受害者名称
	 * 
	 * 创建击杀信息并添加到滚动列表，旧消息会向上移动
	 */
	void AddElimAnnouncement(FString Attacker, FString Victim);

protected:
	/** 游戏开始时初始化 */
	virtual void BeginPlay() override;

private:
	/** 拥有此HUD的玩家控制器 */
	UPROPERTY()
	APlayerController* OwningPlayer;

	/** 当前HUD包数据（由外部更新） */
	FHUDPackage HUDPackage;

	/**
	 * @brief 绘制单个准星纹理
	 * @param Texture 准星纹理
	 * @param ViewportCenter 视口中心点
	 * @param Spread 偏移量（控制准星展开）
	 * @param CrosshairColor 准星颜色
	 */
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	/** 准星最大扩散值（像素） */
	UPROPERTY(EditAnywhere, Category = "Crosshair")
	float CrosshairSpreadMax = 16.f;

	/** 击杀公告Widget类 */
	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UElimAnnouncement> ElimAnnouncementClass;

	/** 击杀公告显示时长（秒） */
	UPROPERTY(EditAnywhere, Category = "Announcements")
	float ElimAnnouncementTime = 2.5f;

	/**
	 * @brief 击杀公告定时器结束回调
	 * @param MsgToRemove 需要移除的公告Widget
	 */
	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

	/** 当前显示的击杀公告列表 */
	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;

public:
	/** 设置HUD包数据 */
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
