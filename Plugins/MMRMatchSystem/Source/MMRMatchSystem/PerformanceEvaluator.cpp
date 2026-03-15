/**
 * @file PerformanceEvaluator.cpp
 * @brief 表现评估器实现
 */

#include "PerformanceEvaluator.h"

float UPerformanceEvaluator::EvaluatePerformance_Implementation(
	const TArray<FGameEvent>& PlayerEvents,
	const FString& Role) const
{
	// 无事件返回默认表现分
	if (PlayerEvents.IsEmpty()) return 0.5f;
	
	float TotalScore = 0.0f;
	float TotalWeight = 0.0f;
	
	// 遍历所有事件计算加权得分
	for (const FGameEvent& Event : PlayerEvents)
	{
		float Weight = 1.f;
		
		// 优先使用角色特定权重
		if (!Role.IsEmpty() && RoleSpecificWeights.Contains(Role))
		{
			const FRoleWeightConfig& RoleWeights = RoleSpecificWeights[Role];
			if (RoleWeights.EventWeights.Contains(Event.EventType))
			{
				Weight = RoleWeights.EventWeights[Event.EventType];
			}
		}
		// 否则使用通用权重
		else if (TotalEventWeights.EventWeights.Contains(Event.EventType))
		{
			Weight = TotalEventWeights.EventWeights[Event.EventType];
		}
		
		// 累加加权分数
		TotalScore += Event.Value * Weight;
		TotalWeight += Weight;
	}
	
	// 无效权重返回默认值
	if (TotalWeight <= 0) return 0.5f;
	
	// 归一化到0-1范围
	float RawScore = TotalScore / TotalWeight;
	return FMath::Clamp(RawScore, 0.0f, 1.0f);
}
