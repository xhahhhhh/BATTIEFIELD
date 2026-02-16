
#include "PlayerRatingComponent.h"

UPlayerRatingComponent::UPlayerRatingComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

}

void UPlayerRatingComponent::BeginPlay()
{
	Super::BeginPlay();

}


void UPlayerRatingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

