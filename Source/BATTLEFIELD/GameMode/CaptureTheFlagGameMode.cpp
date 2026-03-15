
/**
 * @file CaptureTheFlagGameMode.cpp
 * @brief 夺旗模式游戏模式实现
 * 
 * 实现夺旗玩法的核心逻辑：
 * - 玩家需要夺取敌方旗帜并带回己方区域
 * - 只有将敌方旗帜带回己方区域才算有效得分
 */

#include "CaptureTheFlagGameMode.h"

#include "BATTLEFIELD/FlagProperty/FlagZone.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/Weapon/Flag.h"
#include "Kismet/GameplayStatics.h"

void ACaptureTheFlagGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
                                               ABasePlayerController* VictimController, 
											 ABasePlayerController* AttackerController)
{
	// 调用父类实现（团队计分等）
	APlayerGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(class AFlag* Flag, class AFlagZone* Zone)
{
	// 验证捕获是否有效：旗帜队伍必须与区域队伍不同
	// （即：必须将敌方旗帜带回己方区域）
	bool bValidCapture = Flag->GetTeam() != Zone->Team;

	if (bValidCapture)
	{
		// 获取游戏状态以更新团队分数
		ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
		if (BGameState)
		{
			// 根据区域所属队伍增加相应分数
			if (Zone->Team == ETeam::ET_BlueTeam)
			{
				BGameState->BlueTeamScores();
			}
			if (Zone->Team == ETeam::ET_RedTeam)
			{
				BGameState->RedTeamScores();
			}
		}
	}
}

float ACaptureTheFlagGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	// 调用父类实现，保持友军伤害免疫
	return Super::CalculateDamage(Attacker, Victim, BaseDamage);
}
