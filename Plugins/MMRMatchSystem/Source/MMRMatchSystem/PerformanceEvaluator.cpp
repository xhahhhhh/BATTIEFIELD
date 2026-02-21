
#include "PerformanceEvaluator.h"

float UPerformanceEvaluator::EvaluatePerformance_Implementation(const TArray<FGameEvent>& PlayerEvents,
	const FString& Role) const
{
	//无事件返回默认值
	if (PlayerEvents.IsEmpty()) return 0.5f;
	
	float TotalScore = 0.0f;
	float TotalWeight = 0.0f;
	
	for (const FGameEvent& Event : PlayerEvents)
	{
		float Weight = 1.f;
		
		//获取角色表现权重
		if (!Role.IsEmpty()&&RoleSpecificWeights.Contains(Role))
		{
			const FRoleWeightConfig& RoleWeights = RoleSpecificWeights[Role];
			if (RoleWeights.EventWeights.Contains(Event.EventType))
				Weight = RoleWeights.EventWeights[Event.EventType];
		}
		//否则获取事件权重
		else if (TotalEventWeights.EventWeights.Contains(Event.EventType))
		{
			Weight = TotalEventWeights.EventWeights[Event.EventType];
		}
		TotalScore += Event.Value * Weight;
		TotalWeight += Weight;
	}
	//如果总权重小于0返回默认值
	if (TotalWeight <= 0) return 0.5f;
	
	//归一化
	float RawScore = TotalScore / TotalWeight;
	return FMath::Clamp(RawScore, 0.0f, 1.0f);

}
