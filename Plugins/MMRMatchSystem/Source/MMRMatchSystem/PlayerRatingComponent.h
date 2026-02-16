
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerRatingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MMRMATCHSYSTEM_API UPlayerRatingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerRatingComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
