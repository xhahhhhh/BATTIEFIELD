// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file MMRMatchSystem.h
 * @brief MMR匹配系统插件模块接口
 * 
 * 这是一个UE插件的入口模块，负责：
 * - 插件加载时的初始化
 * - 插件卸载时的清理
 * 
 * 该插件提供完整的MMR（Matchmaking Rating）匹配系统，包括：
 * - 玩家评分计算（ELO/MMR算法）
 * - 匹配队列管理
 * - 匹配策略实现
 * - 赛季管理
 */

#pragma once

#include "Modules/ModuleManager.h"

/**
 * @brief MMR匹配系统模块类
 * 
 * 实现IModuleInterface接口，管理插件生命周期
 */
class FMMRMatchSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	
	/**
	 * @brief 模块启动时调用
	 * 
	 * 插件加载到内存后执行初始化代码
	 * 具体时机在.uplugin文件中每个模块的配置中指定
	 */
	virtual void StartupModule() override;
	
	/**
	 * @brief 模块关闭时调用
	 * 
	 * 用于清理资源。
	 * 对于支持动态重载的模块，在卸载前调用此函数
	 */
	virtual void ShutdownModule() override;
};
