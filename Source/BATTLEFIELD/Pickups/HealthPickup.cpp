#include "HealthPickup.h"

#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "BATTLEFIELD/Components/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
	
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                    const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OtherActor);
	if (BaseCharacter)
	{
		UBuffComponent* Buff = BaseCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount,HealingTime);
		}
	}
	Destroy();
}

