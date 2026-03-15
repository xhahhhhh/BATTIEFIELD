/**
 * @file NetConnectSessions.h
 * @brief 多人网络会话插件模块头文件
 * @details 定义了NetConnectSessions插件的主模块类，负责插件的启动和关闭
 */

#pragma once

#include "Modules/ModuleManager.h"

/**
 * @class FNetConnectSessionsModule
 * @brief NetConnectSessions插件主模块类
 * @details 继承自IModuleInterface，实现插件生命周期管理
 */
class FNetConnectSessionsModule : public IModuleInterface
{
public:

	/** 
	 * @brief 模块启动时调用
	 * @details 插件加载到内存后执行初始化操作
	 */
	virtual void StartupModule() override;
	
	/** 
	 * @brief 模块关闭时调用
	 * @details 用于清理资源，支持动态重载的模块会在卸载前调用此函数
	 */
	virtual void ShutdownModule() override;
};
