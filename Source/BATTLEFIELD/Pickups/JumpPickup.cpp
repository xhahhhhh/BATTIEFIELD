/**
 * @file JumpPickup.cpp
 * @brief 跳跃增益拾取物实现
 * 
 * 实现跳跃增益拾取的逻辑：
 * - 检测玩家重叠
 * - 通过BuffComponent启动临时跳跃增益
 * - 销毁自身
 */

#include "JumpPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/BuffComponent.h"

AJumpPickup::AJumpPickup()
{
	// 构造函数，可在此添加初始化逻辑
}

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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
			// 启动临时跳跃增益效果
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	
	// 拾取后销毁
	Destroy();
}
