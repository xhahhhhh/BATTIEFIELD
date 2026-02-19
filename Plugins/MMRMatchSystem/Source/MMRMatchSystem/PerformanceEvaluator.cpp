
#include "PerformanceEvaluator.h"

float UPerformanceEvaluator::EvaluatePerformance_Implementation(const TArray<FGameEvent>& PlayerEvents,
	const FString& Role) const
{
	if (PlayerEvents.IsEmpty()) return 0.5f;
	
	float TotalScore = 0.0f;
	float TotalWeight = 0.0f;
	
	for (const FGameEvent& Event : PlayerEvents)
	{
		float Weight = 1.f;
		
		if (!Role.IsEmpty()&&RoleSpecificWeights.Contains(Role))
		{
			const FRoleWeightConfig& RoleWeights = RoleSpecificWeights[Role];
			if (RoleWeights.EventWeights.Contains(Event.EventType))
				Weight = RoleWeights.EventWeights[Event.EventType];
		}
		else if (TotalEventWeights.EventWeights.Contains(Event.EventType))
		{
			Weight = TotalEventWeights.EventWeights[Event.EventType];
		}
		TotalScore += Event.Value * Weight;
		TotalWeight += Weight;
	}
	if (TotalWeight <= 0) return 0.5f;
	
	float RawScore = TotalScore / TotalWeight;
	return FMath::Clamp(RawScore, 0.0f, 1.0f);

}
