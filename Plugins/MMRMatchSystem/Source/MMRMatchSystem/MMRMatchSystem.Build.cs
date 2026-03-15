// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @file MMRMatchSystem.Build.cs
 * @brief MMR匹配系统插件构建设置
 * 
 * 定义插件的编译依赖和配置
 */

using UnrealBuildTool;

public class MMRMatchSystem : ModuleRules
{
	public MMRMatchSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		// 使用显式或共享预编译头
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// 公共包含路径
		PublicIncludePaths.AddRange(
			new string[] {
				// ... 添加需要的公共包含路径 ...
			}
		);
				
		// 私有包含路径
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... 添加需要的私有包含路径 ...
			}
		);
		
		// 公共依赖模块
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... 其他公共依赖模块 ...
			}
		);
		
		// 私有依赖模块
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... 其他私有依赖模块 ...
			}
		);
		
		// 动态加载模块
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... 动态加载的模块 ...
			}
		);
	}
}
