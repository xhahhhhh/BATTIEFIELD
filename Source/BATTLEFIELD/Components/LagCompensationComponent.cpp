#include "LagCompensationComponent.h"

#include "BATTLEFIELD/BATTLEFIELD.h"
#include "BATTLEFIELD/Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

/**
 * 构造函数
 * 启用组件Tick，用于持续记录帧历史
 */
ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/**
 * 组件开始播放
 */
void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * 在两个历史帧之间进行插值计算
 * 当射击时间位于两个记录帧之间时，线性插值计算碰撞盒的精确位置和旋转
 * 
 * @param OlderFrame - 较旧的帧（时间较早）
 * @param YoungerFrame - 较新的帧（时间较晚）
 * @param HitTime - 射击发生的目标时间
 * @return 插值后的帧数据包
 */
FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame,
                                                             const FFramePackage& YoungerFrame, float HitTime)
{
	// 计算两帧之间的时间差
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	// 计算HitTime在两帧之间的比例（0.0 - 1.0）
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.0f, 1.0f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	InterpFramePackage.Character = OlderFrame.Character;
	
	// 对每个碰撞盒进行插值
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;
		// 使用VInterpTo进行位置插值（线性插值）
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		// 使用RInterpTo进行旋转插值（球面插值）
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		// 尺寸保持不变
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}
	return InterpFramePackage;
}

/**
 * 确认命中 - 即时命中武器（射线检测）
 * 
 * 执行流程：
 * 1. 缓存当前碰撞盒位置
 * 2. 将碰撞盒移动到历史位置（回滚）
 * 3. 禁用角色网格碰撞（避免干扰）
 * 4. 先检测头部碰撞盒（爆头判定）
 * 5. 如未命中头部，检测其他碰撞盒
 * 6. 恢复碰撞盒到当前位置
 * 
 * @param Package - 回滚目标帧数据
 * @param HitCharacter - 被检测角色
 * @param TraceStart - 射线起点
 * @param HitLocation - 命中位置
 * @return 命中结果
 */
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package,
                                                              ABaseCharacter* HitCharacter,
                                                              const FVector_NetQuantize& TraceStart,
                                                              const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	// 保存当前状态用于后续恢复
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	// 回滚到历史位置
	MoveBoxes(HitCharacter, Package);
	// 禁用网格碰撞避免干扰
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// 首先只启用头部碰撞盒进行检测（爆头判定优先）
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	FHitResult ConfirmHitResult;
	// 延长射线确保能检测到目标（客户端报告的HitLocation可能有误差）
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.5f;
	
	UWorld* World = GetWorld();
	if (World)
	{
		// 执行射线检测
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
		if (ConfirmHitResult.bBlockingHit)
		{
			// 命中头部，恢复位置并返回结果
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{true, true};
		}
		else // 未命中头部，检查身体其他部位
		{
			// 启用所有碰撞盒
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
				}
			}
			// 再次检测
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_Hitbox);
			if (ConfirmHitResult.bBlockingHit)
			{
				// 命中身体，恢复位置并返回结果
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{true, false};
			}
		}
	}
	// 未命中，恢复位置
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

/**
 * 确认命中 - 抛射物武器（弹道预测）
 * 
 * 使用UGameplayStatics::PredictProjectilePath预测弹道轨迹
 * 适用于火箭筒、榴弹等有弹道的武器
 * 
 * @param Package - 回滚目标帧数据
 * @param HitCharacter - 被检测角色
 * @param TraceStart - 发射起点
 * @param InitialVelocity - 初始速度
 * @param HitTime - 射击时间
 * @return 命中结果
 */
FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package,
                                                                        ABaseCharacter* HitCharacter,
                                                                        const FVector_NetQuantize& TraceStart,
                                                                        const FVector_NetQuantize100& InitialVelocity,
                                                                        float HitTime)
{
	// 保存当前状态
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	// 回滚到历史位置
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// 首先只启用头部碰撞盒
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);

	// 设置弹道预测参数
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;           // 启用碰撞检测
	PathParams.MaxSimTime = MaxRecordTime;           // 最大模拟时间
	PathParams.LaunchVelocity = InitialVelocity;     // 初始速度
	PathParams.StartLocation = TraceStart;           // 起点
	PathParams.SimFrequency = 15.f;                  // 模拟频率（步数/秒）
	PathParams.ProjectileRadius = 5.f;               // 抛射物半径
	PathParams.TraceChannel = ECC_Hitbox;            // 碰撞通道
	PathParams.ActorsToIgnore.Add(GetOwner());       // 忽略发射者
	PathParams.DrawDebugTime = 5.f;                  // 调试绘制时间
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// 检测是否命中头部
	if (PathResult.HitResult.bBlockingHit)
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{true, true};
	}
	else
	{
		// 未命中头部，启用所有碰撞盒重新检测
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
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{true, false};
		}
	}
	
	// 未命中，恢复位置
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false, false};
}

/**
 * 确认命中 - 霰弹枪（多弹丸、多目标）
 * 
 * 霰弹枪一发包含多个弹丸，可能命中多个目标
 * 需要分别统计每个目标的爆头和身体命中次数
 * 
 * @param FramePackages - 多个目标的帧数据数组
 * @param TraceStart - 射线起点
 * @param HitLocations - 多个弹丸命中位置数组
 * @return 霰弹枪命中结果统计
 */
FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
                                                                            const FVector_NetQuantize& TraceStart,
                                                                            const TArray<FVector_NetQuantize>&
                                                                            HitLocations)
{
	// 验证所有目标角色有效
	for (auto& Frame : FramePackages)
	{
		if (Frame.Character == nullptr) return FShotgunServerSideRewindResult();
	}
	
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	
	// 保存所有目标的当前状态
	for (auto& Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}
	
	// 为所有目标启用头部碰撞盒
	for (auto& Frame : FramePackages)
	{
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName("head")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_Hitbox, ECR_Block);
	}
	
	UWorld* World = GetWorld();
	
	// 第一遍检测：检查爆头
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
				// 统计爆头次数
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

	// 切换碰撞设置：禁用头部，启用身体
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

	// 第二遍检测：检查身体命中
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
				// 统计身体命中次数
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
	
	// 恢复所有目标的位置
	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	return ShotgunResult;
}

/**
 * 缓存当前碰撞盒位置
 * 在回滚前保存当前状态，用于后续恢复
 * 
 * @param HitCharacter - 目标角色
 * @param OutFramePackage - 输出的当前状态
 */
void ULagCompensationComponent::CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;
	
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

/**
 * 移动碰撞盒到历史位置
 * 将角色的所有碰撞盒移动到指定的历史状态
 * 
 * @param HitCharacter - 目标角色
 * @param Package - 目标帧数据
 */
void ULagCompensationComponent::MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	
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

/**
 * 重置碰撞盒位置
 * 将碰撞盒恢复到之前缓存的状态，并禁用碰撞
 * 
 * @param HitCharacter - 目标角色
 * @param Package - 要恢复的帧数据
 */
void ULagCompensationComponent::ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	
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

/**
 * 启用/禁用角色网格碰撞
 * 在回滚检测时临时禁用网格碰撞，避免与碰撞盒检测产生干扰
 * 
 * @param HitCharacter - 目标角色
 * @param CollisionEnabled - 碰撞启用状态
 */
void ULagCompensationComponent::EnableCharacterMeshCollision(ABaseCharacter* HitCharacter,
                                                             ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

/**
 * 可视化显示帧数据包
 * 在世界空间中绘制所有碰撞盒，用于调试
 * 
 * @param Package - 要显示的帧数据包
 * @param Color - 绘制颜色
 */
void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat(BoxInfo.Value.Rotation), Color,
		             false, 4.f);
	}
}

/**
 * 服务器端回滚 - 即时命中武器
 * 供外部调用的接口，内部调用GetFrameToCheck获取目标帧，然后执行ConfirmHit
 * 
 * @param HitCharacter - 被射击的目标角色
 * @param TraceStart - 射线起点
 * @param HitLocation - 客户端报告的命中位置
 * @param HitTime - 射击发生时间
 * @return 命中结果
 */
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(class ABaseCharacter* HitCharacter,
                                                                    const FVector_NetQuantize& TraceStart,
                                                                    const FVector_NetQuantize& HitLocation,
                                                                    float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

/**
 * 服务器端回滚 - 抛射物武器
 * 
 * @param HitCharacter - 被射击的目标角色
 * @param TraceStart - 发射起点
 * @param InitialVelocity - 初始速度
 * @param HitTime - 射击发生时间
 * @return 命中结果
 */
FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABaseCharacter* HitCharacter,
                                                                              const FVector_NetQuantize& TraceStart,
                                                                              const FVector_NetQuantize100&
                                                                              InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

/**
 * 服务器端回滚 - 霰弹枪
 * 
 * @param HitCharacters - 被射击的目标角色数组
 * @param TraceStart - 射线起点
 * @param HitLocations - 多个弹丸的命中位置数组
 * @param HitTime - 射击发生时间
 * @return 霰弹枪命中结果
 */
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

/**
 * 获取要检测的帧数据
 * 
 * 根据射击时间从帧历史中找到对应的帧：
 * 1. 如果时间恰好匹配某帧，直接返回该帧
 * 2. 如果时间在两帧之间，进行插值计算
 * 3. 如果时间超出记录范围，返回空帧（判定失败）
 * 
 * 使用快慢指针算法在双端链表中查找目标帧
 * 
 * @param HitCharacter - 目标角色（需要获取其LagCompensation组件的帧历史）
 * @param HitTime - 射击时间
 * @return 用于检测的帧数据
 */
FFramePackage ULagCompensationComponent::GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime)
{
	// 验证参数有效性
	bool bReturn = HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();

	// 用于返回的帧数据
	FFramePackage FrameToCheck;
	bool bShouldInterplate = true;
	
	// 获取目标角色的帧历史
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldsetHistoryTime = History.GetTail()->GetValue().Time;
	const float NewsetHistoryTime = History.GetHead()->GetValue().Time;
	
	// 检查时间是否超出记录范围
	if (OldsetHistoryTime > HitTime)
	{
		// 回溯过久，不予倒带（射击时间比最早记录还早）
		return FFramePackage();
	}
	if (OldsetHistoryTime == HitTime)
	{
		// 恰好匹配最早帧
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterplate = false;
	}
	if (NewsetHistoryTime <= HitTime)
	{
		// 射击时间比最新帧还新（延迟极小或负延迟）
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterplate = false;
	}

	// 快慢指针查找：找到HitTime位于哪两个节点之间
	// Younger指向较新的帧，Older指向较旧的帧
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	
	// 移动指针直到Older的时间 <= HitTime
	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	
	// 检查是否恰好匹配Older的时间
	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterplate = false;
	}
	
	// 需要插值：HitTime位于Older和Younger之间
	if (bShouldInterplate)
	{
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

/**
 * 服务器RPC - 请求计分（即时命中武器）
 * 
 * 客户端在检测到命中后调用此RPC，服务器进行验证并造成伤害
 * 这是延迟补偿的核心：客户端先显示效果，服务器验证后确认伤害
 * 
 * @param HitCharacter - 被命中角色
 * @param TraceStart - 射线起点
 * @param HitLocation - 命中位置
 * @param HitTime - 射击时间
 */
void ULagCompensationComponent::ServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart,
                                                                  const FVector_NetQuantize& HitLocation, float HitTime)
{
	// 服务器执行回滚验证
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	// 验证通过，造成伤害
	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		// 根据是否爆头选择伤害值
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
		
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(),
		                              UDamageType::StaticClass());
	}
}

/**
 * 服务器RPC - 请求计分（抛射物武器）
 * 
 * @param HitCharacter - 被命中角色
 * @param TraceStart - 发射起点
 * @param InitialVelocity - 初始速度
 * @param HitTime - 射击时间
 */
void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABaseCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	// 执行抛射物回滚验证
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	
	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon())
	{
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(),
									  UDamageType::StaticClass());
	}
}

/**
 * 服务器RPC - 请求计分（霰弹枪）
 * 
 * @param HitCharacters - 被命中的角色数组
 * @param TraceStart - 射线起点
 * @param HitLocations - 多个弹丸命中位置
 * @param HitTime - 射击时间
 */
void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABaseCharacter*>& HitCharacters,
                                                                         const FVector_NetQuantize& TraceStart,
                                                                         const TArray<FVector_NetQuantize>&
                                                                         HitLocations, float HitTime)
{
	// 执行霰弹枪回滚验证
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	// 对每个被命中的角色造成伤害
	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetEquippedWeapon() == nullptr || Character == nullptr) continue;
		
		float TotalDamage = 0.f;
		
		// 计算爆头伤害
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		
		// 计算身体伤害
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		
		// 应用总伤害
		UGameplayStatics::ApplyDamage(HitCharacter, TotalDamage, Character->Controller, Character->GetEquippedWeapon(),
		                              UDamageType::StaticClass());
	}
}

/**
 * 每帧Tick函数
 * 持续调用SaveFramePackage()记录帧历史
 */
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

/**
 * 保存帧数据包（无参数版本）
 * 
 * 管理帧历史记录：
 * 1. 只在服务器执行（HasAuthority检查）
 * 2. 保持帧历史在MaxRecordTime时间范围内
 * 3. 移除超出时间范围的旧帧
 * 
 * 这是延迟补偿的数据基础，每秒记录多帧用于后续回滚
 */
void ULagCompensationComponent::SaveFramePackage()
{
	// 只在服务器记录
	if (Character == nullptr || !Character->HasAuthority()) return;
	
	// 第一帧直接添加
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		// 移除超出MaxRecordTime的旧帧
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength >= MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		
		// 添加新帧到头部（最新）
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		
		// ShowFramePackage(ThisFrame, FColor::Red); // 调试用
	}
}

/**
 * 保存帧数据包（带参数版本）
 * 
 * 将当前角色所有碰撞盒的位置、旋转、尺寸信息保存到Package中
 * 
 * @param Package - 输出的帧数据包
 */
void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	// 缓存角色引用
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetOwner()) : Character;
	
	if (Character)
	{
		// 记录当前时间
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		
		// 记录所有碰撞盒信息
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
