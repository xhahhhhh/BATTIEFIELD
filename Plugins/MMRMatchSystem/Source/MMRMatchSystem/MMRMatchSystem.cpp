// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file MMRMatchSystem.cpp
 * @brief MMR匹配系统插件模块实现
 */

#include "MMRMatchSystem.h"

#define LOCTEXT_NAMESPACE "FMMRMatchSystemModule"

void FMMRMatchSystemModule::StartupModule()
{
	// 此代码在模块加载到内存后执行
	// 具体时机在.uplugin文件中的模块配置里指定
}

void FMMRMatchSystemModule::ShutdownModule()
{
	// 此函数在关闭时调用以清理资源
	// 对于支持动态重载的模块，在卸载前调用此函数
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMMRMatchSystemModule, MMRMatchSystem)
