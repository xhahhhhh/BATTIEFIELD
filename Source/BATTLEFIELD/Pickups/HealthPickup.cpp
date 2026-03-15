/**
 * @file HealthPickup.cpp
 * @brief 治疗拾取物实现
 * 
 * 实现治疗拾取的逻辑：
 * - 启用网络复制
 * - 检测玩家重叠
 * - 通过BuffComponent启动持续治疗
 * - 销毁自身
 */

#include "HealthPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	// 启用网络复制，确保治疗效果同步到所有客户端
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                    const FHitResult& SweepResult)
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
			// 启动持续治疗效果
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	
	// 拾取后销毁
	Destroy();
}
