
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	friend class ABaseCharacter;
	void Heal(float HealAmount,float HealingTime);
	void ReplenishShield(float ShieldAmount,float ReplenishTime);
	void BuffSpeed(float BuffBaseSpeed,float BuffCrouchSpeed,float BuffTime);
	void BuffJump(float BuffJumpVelocity,float BuffTime);
	void SetInitialSpeeds(float BaseSpeed,float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);
protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);
private:
	UPROPERTY()
	ABaseCharacter* Character;
	
	//血量
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;
	
	//护盾
	bool bReplenishShield = false;
	float ShieldReplenishRate = 0.f;
	float ShieldReplenishAmount = 0.f;
	
	//速度
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastSpeedBuff(float BaseSpeed,float CrouchSpeed);
	
	//跳跃
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastJumpBuff(float JumpVelocity);
};
