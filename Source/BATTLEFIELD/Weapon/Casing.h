
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class BATTLEFIELD_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere)
	float ShellImpulse;

	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

};
