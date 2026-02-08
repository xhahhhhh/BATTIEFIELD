#include "LagCompensationComponent.h"

#include "BATTLEFIELD/BATTLEFIELD.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame,
                                                             const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	InterpFramePackage.Character = OlderFrame.Character;
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}
	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
                                                              ABaseCharacter* HitCharacter,
                                                              const FVector_NetQuantize& TraceStart,
                                                              const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr)return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	//启用碰撞
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.5f;
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
		if (ConfirmHitResult.bBlockingHit)
		{
			/*if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component.Get());
				if (Box)
				{
					DrawDebugBox(GetWorld(),Box->GetComponentLocation(),Box->GetScaledBoxExtent(),FColor::Red,false,3.f);
				}
			}*/

			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{true, true};
		}
		else //没有击中头部，检查其他部位
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
			if (ConfirmHitResult.bBlockingHit)
			{
				/*if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component.Get());
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FColor::Red,
						             false, 3.f);
					}
				}*/

				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{true, false};
			}
		}
	}
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package,
                                                                        ABaseCharacter* HitCharacter,
                                                                        const FVector_NetQuantize& TraceStart,
                                                                        const FVector_NetQuantize100& InitialVelocity,
                                                                        float HitTime)
{
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	//启用碰撞
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_Hitbox;
	PathParams.ActorsToIgnore.Add(GetOwner());
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	if (PathResult.HitResult.bBlockingHit)
	{
		if (PathResult.HitResult.bBlockingHit)
		{
			/*if (PathResult.HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component.Get());
				if (Box)
				{
					DrawDebugBox(GetWorld(),Box->GetComponentLocation(),Box->GetScaledBoxExtent(),FColor::Red,false,3.f);
				}
			}*/

			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{true, true};
		}
		else
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
				}
			}
			UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
			if (PathResult.HitResult.bBlockingHit)
			{
				/*if (PathResult.HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component.Get());
				if (Box)
				{
					DrawDebugBox(GetWorld(),Box->GetComponentLocation(),Box->GetScaledBoxExtent(),FColor::Red,false,3.f);
				}
			}*/

				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{true, false};
			}
		}
	}
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
                                                                            const FVector_NetQuantize& TraceStart,
                                                                            const TArray<FVector_NetQuantize>&
                                                                            HitLocations)
{
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}
	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
	}
	UWorld* World = GetWorld();
	//检查被霰弹击中头部的玩家
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.5f;
		if (World)
		{
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ConfirmHitResult.GetActor());
			if (BaseCharacter)
			{
				if (ShotgunResult.HeadShots.Contains(BaseCharacter))
				{
					ShotgunResult.HeadShots[BaseCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}

	//禁用头部碰撞并打开身体部位碰撞
	for (auto& Frame : FramePackages)
	{
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
			}
		}
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	//检查身体被击中的玩家
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.5f;
		if (World)
		{
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
			ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ConfirmHitResult.GetActor());
			if (BaseCharacter)
			{
				if (ShotgunResult.BodyShots.Contains(BaseCharacter))
				{
					ShotgunResult.BodyShots[BaseCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace(BaseCharacter, 1);
				}
			}
		}
	}
	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABaseCharacter* HitCharacter,
                                                             ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && Character->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color,
		             false, 4.f);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(class ABaseCharacter* HitCharacter,
                                                                    const FVector_NetQuantize& TraceStart,
                                                                    const FVector_NetQuantize& HitLocation,
                                                                    float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABaseCharacter* HitCharacter,
                                                                              const FVector_NetQuantize& TraceStart,
                                                                              const FVector_NetQuantize100&
                                                                              InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABaseCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABaseCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();

	//用于验证命中的数据
	FFramePackage FrameToCheck;
	bool bShouldInterplate = true;
	// 受击玩家的历史帧包
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldsetHistoryTime = History.GetTail()->GetValue().Time;
	const float NewsetHistoryTime = History.GetHead()->GetValue().Time;
	if (OldsetHistoryTime > HitTime)
	{
		//回溯过久，不予倒带
		return FFramePackage();
	}
	if (OldsetHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterplate = false;
	}
	if (NewsetHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterplate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older->GetValue().Time > HitTime)
	{
		//快慢指针,根据OlderTime是否>HitTime来移动双指针从而找到HitTime在队列中的哪两个节点之间
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	if (Older->GetValue().Time == HitTime) //HitTime与OlderTime重合直接赋值
	{
		FrameToCheck = Older->GetValue();
		bShouldInterplate = false;
	}
	//判断是否插值（HitTime在OlderTime和YoungerTime之间）
	if (bShouldInterplate)
	{
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart,
                                                                  const FVector_NetQuantize& HitLocation, float HitTime)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
		
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(),
		                              UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	if (Character && HitCharacter && Confirm.bHitConfirmed &&Character->GetEquippedWeapon())
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(),
									  UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABaseCharacter*>& HitCharacters,
                                                                         const FVector_NetQuantize& TraceStart,
                                                                         const TArray<FVector_NetQuantize>&
                                                                         HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetEquippedWeapon() == nullptr || Character == nullptr) continue;
		float TotalDamage = 0.f;;
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		UGameplayStatics::ApplyDamage(HitCharacter, TotalDamage, Character->Controller, Character->GetEquippedWeapon(),
		                              UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority())return;
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength >= MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		// ShowFramePackage(ThisFrame,FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}
