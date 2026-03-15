#pragma once

#include "CoreMinimal.h"
#include "BATTLEFIELD/Weapon/WeaponBase.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class AWeaponBase;
class ABaseCharacter;

/** 
 * 碰撞盒信息结构体
 * 存储单个碰撞盒在某一时刻的位置、旋转和尺寸信息
 * 用于延迟补偿系统中记录角色身体部位的历史状态
 */
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	/** 碰撞盒的世界位置 */
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** 碰撞盒的旋转 */
	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	/** 碰撞盒的扩展尺寸（半尺寸） */
	UPROPERTY()
	FVector BoxExtent = FVector::OneVector;
};

/**
 * 帧数据包结构体
 * 存储某一时刻角色的完整碰撞盒状态
 * 用于服务器回滚（Server-Side Rewind）功能
 */
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	/** 记录时间戳（游戏世界时间，秒） */
	UPROPERTY()
	float Time = 0.f;

	/** 碰撞盒名称到信息的映射表（如 "head" -> 头部碰撞盒信息） */
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	/** 关联的角色指针 */
	UPROPERTY()
	ABaseCharacter* Character = nullptr;
};

/**
 * 服务器回滚命中结果
 * 用于即时命中武器（Instant Hit Weapon）和抛射物武器（Projectile）的命中判定结果
 */
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	/** 是否确认命中 */
	UPROPERTY()
	bool bHitConfirmed = false;

	/** 是否为爆头（头部命中） */
	UPROPERTY()
	bool bHeadShot = false;
};

/**
 * 霰弹枪服务器回滚命中结果
 * 霰弹枪需要特殊处理，因为一发可能包含多个弹丸，可能命中多个目标
 * 需要分别统计每个角色的爆头和身体命中次数
 */
USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	/** 每个被命中角色的爆头次数映射（角色 -> 爆头次数） */
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> HeadShots;

	/** 每个被命中角色的身体命中次数映射（角色 -> 身体命中次数） */
	UPROPERTY()
	TMap<ABaseCharacter*, uint32> BodyShots;
};

/**
 * 延迟补偿组件
 * 
 * 功能说明：
 * 在多人射击游戏中解决网络延迟导致的命中判定问题。
 * 当延迟较高的玩家射击时，服务器会回滚到射击发生时刻的状态进行命中判定。
 * 
 * 工作原理：
 * 1. 服务器持续记录每个角色的碰撞盒历史位置（帧历史）
 * 2. 当客户端请求命中验证时，服务器根据射击时间找到对应的历史帧
 * 3. 将目标角色回滚到该时刻的位置，进行命中检测
 * 4. 检测完成后恢复角色当前位置
 * 
 * 支持三种武器类型：
 * - 即时命中武器（射线检测）
 * - 抛射物武器（弹道预测）
 * - 霰弹枪（多弹丸、多目标）
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BATTLEFIELD_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数 */
	ULagCompensationComponent();
	
	friend class ABaseCharacter;

	/**
	 * 每帧Tick函数
	 * 负责调用SaveFramePackage()持续记录帧历史
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 可视化显示帧数据包
	 * 用于调试，在世界空间中绘制所有碰撞盒
	 * @param Package - 要显示的帧数据包
	 * @param Color - 绘制颜色
	 */
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);
	
	/**
	 * 服务器端回滚 - 即时命中武器
	 * 用于普通枪械（步枪、手枪等）的命中验证
	 * @param HitCharacter - 被射击的目标角色
	 * @param TraceStart - 射线起点
	 * @param HitLocation - 客户端报告的命中位置
	 * @param HitTime - 射击发生时间
	 * @return 命中结果（是否命中、是否爆头）
	 */
	FServerSideRewindResult ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                                         const FVector_NetQuantize& HitLocation, float HitTime);

	/**
	 * 服务器端回滚 - 抛射物武器
	 * 用于火箭筒、榴弹等需要弹道预测的武器
	 * @param HitCharacter - 被射击的目标角色
	 * @param TraceStart - 发射起点
	 * @param InitialVelocity - 初始速度
	 * @param HitTime - 射击发生时间
	 * @return 命中结果（是否命中、是否爆头）
	 */
	FServerSideRewindResult ProjectileServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
												 const FVector_NetQuantize100& InitialVelocity, float HitTime);

	/**
	 * 服务器端回滚 - 霰弹枪
	 * 霰弹枪特殊处理：一发多弹丸，可能命中多个目标
	 * @param HitCharacters - 被射击的目标角色数组
	 * @param TraceStart - 射线起点
	 * @param HitLocations - 多个弹丸的命中位置数组
	 * @param HitTime - 射击发生时间
	 * @return 霰弹枪命中结果（每个目标的爆头/身体命中次数）
	 */
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<ABaseCharacter*>& HitCharacters,
													   const FVector_NetQuantize& TraceStart,
													   const TArray<FVector_NetQuantize>& HitLocations,
													   float HitTime);

	/**
	 * 确认命中 - 即时命中武器
	 * 实际的命中检测逻辑，将角色回滚后执行射线检测
	 * @param Package - 回滚目标帧数据
	 * @param HitCharacter - 被检测角色
	 * @param TraceStart - 射线起点
	 * @param HitLocation - 命中位置
	 * @return 命中结果
	 */
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter,
								   const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);

	/**
	 * 确认命中 - 抛射物武器
	 * 使用PredictProjectilePath预测弹道轨迹进行检测
	 * @param Package - 回滚目标帧数据
	 * @param HitCharacter - 被检测角色
	 * @param TraceStart - 发射起点
	 * @param InitialVelocity - 初始速度
	 * @param HitTime - 射击时间
	 * @return 命中结果
	 */	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter,
											 const FVector_NetQuantize& TraceStart,
											 const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	/**
	 * 确认命中 - 霰弹枪
	 * 对多个弹丸分别进行命中检测
	 * @param FramePackages - 多个目标的帧数据数组
	 * @param TraceStart - 射线起点
	 * @param HitLocations - 多个弹丸命中位置
	 * @return 霰弹枪命中结果统计
	 */
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
												 const FVector_NetQuantize& TraceStart,
												 const TArray<FVector_NetQuantize>& HitLocations);

	/**
	 * 服务器RPC - 请求计分（即时命中武器）
	 * 客户端在检测到命中后调用，服务器验证后造成伤害
	 * @param HitCharacter - 被命中角色
	 * @param TraceStart - 射线起点
	 * @param HitLocation - 命中位置
	 * @param HitTime - 射击时间
	 */
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
	                        const FVector_NetQuantize& HitLocation, float HitTime);
	
	/**
	 * 服务器RPC - 请求计分（抛射物武器）
	 * @param HitCharacter - 被命中角色
	 * @param TraceStart - 发射起点
	 * @param InitialVelocity - 初始速度
	 * @param HitTime - 射击时间
	 */
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
															  const FVector_NetQuantize100& InitialVelocity, float HitTime);
	
	/**
	 * 服务器RPC - 请求计分（霰弹枪）
	 * @param HitCharacters - 被命中的角色数组
	 * @param TraceStart - 射线起点
	 * @param HitLocations - 多个弹丸命中位置
	 * @param HitTime - 射击时间
	 */
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(const TArray<ABaseCharacter*>& HitCharacters,
	                               const FVector_NetQuantize& TraceStart,
	                               const TArray<FVector_NetQuantize>& HitLocations,
	                               float HitTime);

protected:
	/** 组件开始播放时调用 */
	virtual void BeginPlay() override;

	/**
	 * 保存帧数据包（带参数版本）
	 * 将当前角色所有碰撞盒信息保存到Package中
	 * @param Package - 输出的帧数据包
	 */
	void SaveFramePackage(FFramePackage& Package);

	/**
	 * 在两个历史帧之间进行插值
	 * 当射击时间恰好位于两个记录帧之间时使用
	 * @param OlderFrame - 较旧的帧
	 * @param YoungerFrame - 较新的帧
	 * @param HitTime - 目标时间
	 * @return 插值后的帧数据
	 */
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame,
	                                  float HitTime);
	
	/**
	 * 缓存当前碰撞盒位置
	 * 在回滚前保存当前状态，用于后续恢复
	 * @param HitCharacter - 目标角色
	 * @param OutFramePackage - 输出的当前状态
	 */
	void CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage);

	/**
	 * 移动碰撞盒到历史位置
	 * 根据帧数据包将角色碰撞盒移动到历史状态
	 * @param HitCharacter - 目标角色
	 * @param Package - 目标帧数据
	 */
	void MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);

	/**
	 * 重置碰撞盒位置
	 * 将碰撞盒恢复到之前缓存的状态
	 * @param HitCharacter - 目标角色
	 * @param Package - 要恢复的帧数据
	 */
	void ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package);

	/**
	 * 启用/禁用角色网格碰撞
	 * 在回滚检测时临时禁用网格碰撞，避免干扰
	 * @param HitCharacter - 目标角色
	 * @param CollisionEnabled - 碰撞启用状态
	 */
	void EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	/**
	 * 保存帧数据包（无参数版本）
	 * 自动创建新的帧并添加到历史记录中
	 */
	void SaveFramePackage();

	/**
	 * 获取要检测的帧数据
	 * 根据射击时间从历史记录中找到对应的帧（或进行插值）
	 * @param HitCharacter - 目标角色（需要获取其LagCompensation组件的帧历史）
	 * @param HitTime - 射击时间
	 * @return 用于检测的帧数据
	 */
	FFramePackage GetFrameToCheck(ABaseCharacter* HitCharacter, float HitTime);

	
private:
	/** 拥有此组件的角色 */
	UPROPERTY()
	ABaseCharacter* Character;

	/** 角色控制器（缓存） */
	UPROPERTY()
	class ABasePlayerController* Controller;

	/** 帧历史记录 - 双端链表，头部是最新帧 */
	TDoubleLinkedList<FFramePackage> FrameHistory;

	/** 最大记录时间（秒）- 超过此时间的旧帧会被移除 */
	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 2.f;
};
