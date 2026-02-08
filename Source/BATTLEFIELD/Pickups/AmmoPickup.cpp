
#include "AmmoPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/CombatComponent.h" 


void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if(BaseCharacter)
	{
		UCombatComponent* Combat = BaseCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType,AmmoAmount);
		}
	}
	Destroy();

}
