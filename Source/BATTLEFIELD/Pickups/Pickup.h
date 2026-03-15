
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * @brief 拾取物基类 (Pickup)
 * 
 * 所有拾取物的基类，提供通用功能：
 * - 球形碰撞检测（仅对Pawn响应）
 * - 持续旋转的视觉效果
 * - 自定义深度渲染（高亮轮廓）
 * - 拾取音效和特效
 * - 延迟绑定重叠事件（防止生成时立即触发）
 */
UCLASS()
class BATTLEFIELD_API APickup : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化组件 */
	APickup();

	/**
	 * @brief 每帧更新
	 * @param DeltaTime 距离上一帧的时间间隔
	 * 
	 * 处理拾取物旋转动画
	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief Actor被销毁时调用
	 * 
	 * 播放拾取音效和特效
	 */
	virtual void Destroyed() override;

protected:
	/** 游戏开始时初始化 */
	virtual void BeginPlay() override;

	/**
	 * @brief 球形碰撞重叠事件
	 * @param OverlappedComponent 被重叠的组件
	 * @param OtherActor 重叠的Actor
	 * @param OtherComp 重叠的组件
	 * @param OtherBodyIndex 重叠的物理体索引
	 * @param bFromSweep 是否来自扫描检测
	 * @param SweepResult 扫描结果
	 * 
	 * 子类重写此函数实现具体的拾取逻辑
	 */
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								 const FHitResult& SweepResult);

	/**
	 * @brief 基础旋转速率（度/秒）
	 * 
	 * 拾取物围绕Y轴的旋转速度
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Visuals")
	float BaseTurnRate = 45.f;

private:
	/**
	 * @brief 球形碰撞组件（触发拾取）
	 * 
	 * 用于检测玩家进入拾取范围
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Collision")
	USphereComponent* OverlapSphere;

	/**
	 * @brief 拾取音效
	 * 
	 * 被拾取时播放的声音
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Audio")
	USoundCue* PickupSound;

	/**
	 * @brief 拾取物网格
	 * 
	 * 可见的3D模型，无碰撞
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Visuals")
	UStaticMeshComponent* PickupMesh;

	/**
	 * @brief 持续特效组件
	 * 
	 * 拾取物存在的持续粒子效果
	 */
	UPROPERTY(VisibleAnywhere, Category = "Pickup Visuals")
	UNiagaraComponent* PickupEffectComponent;

	/**
	 * @brief 拾取爆发特效
	 * 
	 * 被拾取时生成的一次性粒子效果
	 */
	UPROPERTY(EditAnywhere, Category = "Pickup Visuals")
	UNiagaraSystem* PickupEffect;

	/**
	 * @brief 绑定重叠事件的定时器
	 * 
	 * 延迟绑定防止生成时立即触发重叠
	 */
	FTimerHandle BindOverlapTimer;

	/**
	 * @brief 绑定延迟时间（秒）
	 * 
	 * 生成后等待此时间才绑定重叠事件
	 */
	float BindOverlapTime = 0.3f;

	/** 延迟绑定定时器完成回调 */
	void BindOverlapTimerFinished();
};
