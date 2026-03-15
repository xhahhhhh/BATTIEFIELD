#pragma once

/**
 * @file WeaponTypes.h
 * @brief 武器类型定义和常量
 * 
 * 定义武器相关的常量值和武器类型枚举
 */

/** 射线检测长度（厘米） */
#define TRACE_LENGTH 80000.f

/** 自定义深度值：紫色（用于高亮显示） */
#define CUSTOM_DEPTH_PURPLE 250

/** 自定义深度值：蓝色（用于武器高亮） */
#define CUSTOM_DEPTH_BLUE 251

/** 自定义深度值：棕褐色 */
#define CUSTOM_DEPTH_TAN 252

/**
 * @brief 武器类型枚举
 * 
 * 定义游戏中所有可用的武器类型
 */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "AssaultRifle"),       // 突击步枪
	EWT_RocketLauncher UMETA(DisplayName = "RocketLauncher"),   // 火箭发射器
	EWT_Pistol UMETA(DisplayName = "Pistol"),                   // 手枪
	EWT_SMG UMETA(DisplayName = "SMG"),                         // 冲锋枪
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),                 // 霰弹枪
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),        // 狙击步枪
	EWT_GrenadeLauncher UMETA(DisplayName = "GrenadeLauncher"), // 榴弹发射器
	EWT_Flag UMETA(DisplayName = "Flag"),                       // 旗帜（夺旗模式）

	EWT_MAX UMETA(DisplayName = "DefaultMAX")                   // 最大值（用于遍历）
};
