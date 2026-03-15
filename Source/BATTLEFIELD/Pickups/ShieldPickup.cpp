/**
 * @file ShieldPickup.cpp
 * @brief 护盾拾取物实现
 * 
 * 实现护盾拾取的逻辑：
 * - 检测玩家重叠
 * - 通过BuffComponent启动持续护盾恢复
 * - 销毁自身
 */

#include "ShieldPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/BuffComponent.h"

AShieldPickup::AShieldPickup()
{
	// 构造函数，可在此添加初始化逻辑
}

void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 调用父类实现（当前为空）
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// 尝试将重叠的Actor转换为玩家角色
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		// 获取玩家的增益组件
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff)
		{
			// 启动持续护盾恢复效果
			Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
		}
	}
	
	// 拾取后销毁
	Destroy();
}
