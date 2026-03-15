
#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"

class AFlag;
class AFlagZone;

/**
 * @brief 夺旗模式游戏模式 (CaptureTheFlagGameMode)
 * 
 * 继承自TeamsGameMode，实现夺旗玩法：
 * - 旗帜捕获计分
 * - 验证旗帜捕获（不能将己方旗帜带回己方区域）
 * 
 * 玩家需要将敌方旗帜带回己方区域来获得团队分数
 */
UCLASS()
class BATTLEFIELD_API ACaptureTheFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:
	/**
	 * @brief 处理玩家被淘汰事件
	 * @param EliminatedCharacter 被淘汰的角色
	 * @param VictimController 受害者控制器
	 * @param AttackerController 攻击者控制器
	 */
	virtual void PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
	                              ABasePlayerController* VictimController,
	                              ABasePlayerController* AttackerController) override;

	/**
	 * @brief 处理旗帜被捕获事件
	 * @param Flag 被捕获的旗帜
	 * @param Zone 旗帜被带入的区域
	 * 
	 * 验证捕获是否有效（必须是敌方旗帜带回己方区域），
	 * 并为相应队伍增加分数
	 */
	void FlagCaptured(class AFlag* Flag, class AFlagZone* Zone);

	/**
	 * @brief 计算实际伤害值
	 * @param Attacker 攻击者控制器
	 * @param Victim 受害者控制器
	 * @param BaseDamage 基础伤害值
	 * @return 实际应用的伤害值
	 * 
	 * 当前实现直接调用父类（支持友军免疫）
	 */
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
};
