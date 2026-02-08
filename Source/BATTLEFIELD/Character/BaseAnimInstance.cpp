#include "BaseAnimInstance.h"
#include "BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Weapon/WeaponBase.h"
#include "../CharacterTypes/CombatState.h"
#include "Kismet/KismetMathLibrary.h"

void UBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
}

void UBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningCharacter == nullptr)
	{
		OwningCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
	}
	if (OwningCharacter == nullptr)return;

	Speed = OwningCharacter->GetVelocity().Size2D();
	bIsInAir = OwningCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = OwningCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size2D() > 0.f ? true : false;
	bWeaponEquipped = OwningCharacter->IsWeaponEquipped();
	EquippedWeapon = OwningCharacter->GetEquippedWeapon();
	bIsCrouched = OwningCharacter->bIsCrouched;
	bAiming = OwningCharacter->IsAiming();
	TurningInPlace = OwningCharacter->GetTurningInPlace();
	bRotateRootBone = OwningCharacter->ShouldRotateRootBone();
	bElimmed = OwningCharacter->IsElimmed();
	bHoldingTheFlag = OwningCharacter->IsHoldingTheFlag();

	//��������ʱ��ƫ��
	FRotator AimRotation = OwningCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(OwningCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = OwningCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
	AO_Yaw = OwningCharacter->GetAO_Yaw();
	AO_Pitch = OwningCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && OwningCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(
			FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		OwningCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(),
		                                                 FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (OwningCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(
				FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - OwningCharacter->
					GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}

		bUseFABRIK = OwningCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
		bool bFabrik = OwningCharacter->IsLocallyControlled() && OwningCharacter->GetCombatState() !=
			ECombatState::ECS_ThrowingGrenade && OwningCharacter->GetCombatState() != ECombatState::ECS_SwappingWeapons && OwningCharacter->bFinishedSwapping;
		if (bFabrik)
		{
			bUseFABRIK = !OwningCharacter->IsLocallyReloading();
		}
		bUseAimOffsets = OwningCharacter->GetCombatState() != ECombatState::ECS_Reloading && !OwningCharacter->
			GetDisableGameplay();
		bTransformRightHand = OwningCharacter->GetCombatState() != ECombatState::ECS_Reloading && !OwningCharacter->
			GetDisableGameplay();
		/*FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlashSocket"), ERelativeTransformSpace::RTS_World);
		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), OwningCharacter->GetHitTarget(), FColor::Orange);*/
	}
}
