/**
 * @file NetConnectSessions.cpp
 * @brief 多人网络会话插件模块实现文件
 * @details 实现NetConnectSessions插件的主模块生命周期管理
 */

#include "NetConnectSessions.h"

#define LOCTEXT_NAMESPACE "FNetConnectSessionsModule"

/**
 * @brief 模块启动函数
 * @details 插件加载到内存后执行，具体时机在.uplugin文件中配置
 */
void FNetConnectSessionsModule::StartupModule()
{
	// 模块加载后的初始化代码
}

/**
 * @brief 模块关闭函数
 * @details 插件卸载前调用，用于清理资源
 *          支持动态重载的模块会在卸载前执行此函数
 */
void FNetConnectSessionsModule::ShutdownModule()
{
	// 模块关闭前的清理代码
}

#undef LOCTEXT_NAMESPACE
	
/** @brief 实现NetConnectSessions模块 */
IMPLEMENT_MODULE(FNetConnectSessionsModule, NetConnectSessions)