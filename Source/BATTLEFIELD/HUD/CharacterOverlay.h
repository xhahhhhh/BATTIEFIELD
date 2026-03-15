// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UWidgetAnimation;

/**
 * @brief 角色状态覆盖面板 (CharacterOverlay)
 * 
 * 主游戏界面的核心UI面板，显示：
 * - 血量条和数值
 * - 护盾条和数值
 * - 个人分数和死亡数
 * - 团队分数（红队/蓝队）
 * - 武器弹药和携带弹药
 * - 比赛倒计时
 * - 手雷数量
 * - 高延迟警告动画
 * 
 * 所有成员使用 UPROPERTY(meta = (BindWidget)) 
 * 自动绑定到蓝图中的同名组件
 */
UCLASS()
class BATTLEFIELD_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	//============================
	// 血量显示
	//============================
	
	/** 血量进度条 */
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	/** 血量数值文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	//============================
	// 护盾显示
	//============================

	/** 护盾进度条 */
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;

	/** 护盾数值文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText;

	//============================
	// 个人战绩
	//============================

	/** 个人分数 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;

	/** 蓝队分数 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* BlueTeamScore;

	/** 红队分数 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* RedTeamScore;

	/** 分数分隔符（如 "-" 或 ":"） */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreSpacerText;

	/** 死亡次数 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;

	//============================
	// 弹药显示
	//============================

	/** 当前武器弹药 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;

	/** 携带的备用弹药 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;

	//============================
	// 比赛信息
	//============================

	/** 比赛倒计时文本 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	//============================
	// 装备和状态
	//============================

	/** 手雷数量 */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText;

	/** 高延迟警告图标 */
	UPROPERTY(meta = (BindWidget))
	UImage* HighPingImage;

	/**
	 * @brief 高延迟警告动画
	 * 
	 * Transient标记表示不序列化保存
	 * BindWidgetAnim 自动绑定蓝图中的动画
	 */
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* HighPingAnimation;
};
