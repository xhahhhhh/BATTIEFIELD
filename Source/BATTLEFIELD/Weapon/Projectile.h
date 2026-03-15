#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UParticleSystem;
class UBoxComponent;
class USoundCue;

/**
 * @brief 投射物基类 (Projectile)
 * 
 * 所有投射物的基类，提供通用功能：
 * - 碰撞检测和处理
 * - 尾迹特效系统（Niagara）
 * - 命中特效和音效
 * - 范围伤害（爆炸）
 * - 自动销毁机制
 * 
 * 支持服务器倒带补偿，用于高延迟网络环境
 */
UCLASS()
class BATTLEFIELD_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数，设置碰撞和复制属性 */
	AProjectile();

	/** 每帧更新（目前为空实现） */
	virtual void Tick(float DeltaTime) override;

	/** 销毁时调用，播放命中特效和音效 */
	virtual void Destroyed() override;

	//============================
	// 服务器倒带相关属性
	//============================
	/** 是否使用服务器倒带补偿 */
	bool bUseServerSideRewind = false;

	/** 射击起始位置（网络量化压缩） */
	FVector_NetQuantize TraceStart;

	/** 初始速度向量（网络量化压缩） */
	FVector_NetQuantize100 InitialVelocity;

	/** 初始发射速度 */
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	/** 基础伤害值 */
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	/** 爆头伤害值 */
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

protected:
	/** 组件开始播放时调用，生成曳光效果 */
	virtual void BeginPlay() override;

	/** 启动自动销毁定时器 */
	void StartDestroyTimer();

	/** 定时器回调，销毁投射物 */
	void DestroyTimerFinished();

	/** 生成尾迹粒子系统 */
	void SpawnTrailSystem();

	/** 
	 * @brief 执行爆炸范围伤害
	 * 
	 * 使用 ApplyRadialDamageWithFalloff 实现伤害衰减：
	 * - DamageInnerRadius 内：全额伤害
	 * - DamageOuterRadius 外：无伤害
	 * - 之间：线性衰减
	 */
	void ExplodeDamage();

	/**
	 * @brief 碰撞回调函数
	 * @param HitComp 发生碰撞的组件
	 * @param OtherActor 碰撞到的Actor
	 * @param OtherComp 碰撞到的组件
	 * @param NormalImpulse 碰撞法线冲量
	 * @param Hit 命中结果详情
	 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

	/** 命中时播放的粒子特效 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	/** 命中时播放的音效 */
	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	/** 碰撞盒组件（根组件） */
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	/** 投射物移动组件（在子类中创建） */
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	/** 尾迹粒子系统资产 */
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	/** 尾迹粒子组件实例 */
	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	/** 投射物网格组件 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	/** 爆炸伤害内半径（全额伤害） */
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	/** 爆炸伤害外半径（无伤害） */
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

private:
	/** 飞行中的曳光粒子特效 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	/** 曳光粒子组件实例 */
	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	/** 自动销毁定时器句柄 */
	FTimerHandle DestroyTimer;

	/** 自动销毁延迟时间（秒） */
	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
};
