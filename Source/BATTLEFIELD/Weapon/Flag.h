
#pragma once

#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "Flag.generated.h"

UCLASS()
class BATTLEFIELD_API AFlag : public AWeaponBase
{
	GENERATED_BODY()
public:
	AFlag();
	virtual void Dropped() override;
	void ResetFlag();
protected:
	virtual void BeginPlay() override;
	virtual void OnEquipped()override;
	virtual void OnDropped()override;
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;
	
	FTransform InitialTransform;
public:
	FORCEINLINE FTransform GetInitialTransform() const  { return InitialTransform; }
};
