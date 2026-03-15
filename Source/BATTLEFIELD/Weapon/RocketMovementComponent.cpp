/**
 * @file RocketMovementComponent.cpp
 * @brief 火箭移动组件实现
 */

#include "RocketMovementComponent.h"

URocketMovementComponent::EHandleBlockingHitResult URocketMovementComponent::HandleBlockingHit(
	const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	// 调用父类实现
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	// 返回 AdvanceNextSubstep 确保碰撞后继续移动
	// 这对火箭弹很重要，因为我们需要 OnHit 被调用以触发爆炸
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void URocketMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// 调用父类实现处理碰撞
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
}
