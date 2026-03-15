#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

/**
 * @brief 弹壳类 (Casing)
 * 
 * 枪械射击时弹出的弹壳视觉效果
 * 
 * 特性：
 * - 物理模拟（重力、碰撞）
 * - 弹出时施加侧向冲量
 * - 落地时播放音效并销毁
 */
UCLASS()
class BATTLEFIELD_API ACasing : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数，创建网格和设置物理属性 */
	ACasing();

private:
	/** 弹壳网格组件 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	/** 弹壳弹出冲量大小 */
	UPROPERTY(EditAnywhere)
	float ShellImpulse;

	/** 落地时播放的音效 */
	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;

protected:
	/** 
	 * @brief 组件开始播放时调用
	 * 
	 * 绑定碰撞事件并施加侧向冲量
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 碰撞回调函数
	 * @param HitComp 碰撞组件
	 * @param OtherActor 碰撞到的Actor
	 * @param OtherComp 碰撞到的组件
	 * @param NormalImpulse 法线冲量
	 * @param Hit 命中结果
	 * 
	 * 落地时播放音效并销毁弹壳
	 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);
};
