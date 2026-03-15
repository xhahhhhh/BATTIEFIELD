/**
 * @file AmmoPickup.cpp
 * @brief 弹药拾取物实现
 * 
 * 实现弹药拾取的逻辑：
 * - 检测玩家重叠
 * - 通过CombatComponent补充指定武器弹药
 * - 销毁自身
 */

#include "AmmoPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 调用父类实现（当前为空）
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	// 尝试将重叠的Actor转换为玩家角色
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		// 获取玩家的战斗组件
		UCombatComponent* Combat = BaseCharacter->GetCombat();
		if (Combat)
		{
			// 为目标武器补充弹药
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	
	// 拾取后销毁
	Destroy();
}
