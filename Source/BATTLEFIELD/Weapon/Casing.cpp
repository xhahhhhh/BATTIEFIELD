/**
 * @file Casing.cpp
 * @brief 弹壳实现
 */

#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	// 禁用Tick，不需要每帧更新
	PrimaryActorTick.bCanEverTick = false;

	// 创建弹壳网格
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);

	// 设置碰撞和物理属性
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	// 默认冲量大小
	ShellImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	// 绑定碰撞事件
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);

	// 施加侧向冲量（向右弹出）
	CasingMesh->AddImpulse(GetActorRightVector() * ShellImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                    FVector NormalImpulse, const FHitResult& Hit)
{
	// 落地时播放音效
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	// 销毁弹壳
	Destroy();
}
