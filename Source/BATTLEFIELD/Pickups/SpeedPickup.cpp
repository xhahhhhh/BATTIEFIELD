
#include "SpeedPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/BuffComponent.h"


ASpeedPickup::ASpeedPickup()
{
}

void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffSpeed(BaseSpeedBuff,CrouchSpeedBuff,SpeedBuffTime);
		}
	}
	Destroy();

}



