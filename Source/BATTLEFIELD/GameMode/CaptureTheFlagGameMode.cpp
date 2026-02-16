
#include "CaptureTheFlagGameMode.h"

#include "BATTLEFIELD/FlagProperty/FlagZone.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/Weapon/Flag.h"
#include "Kismet/GameplayStatics.h"

class ABaseGameState;

void ACaptureTheFlagGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
                                               ABasePlayerController* VictimController, ABasePlayerController* AttackerController)
{
	APlayerGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
}

void ACaptureTheFlagGameMode::FlagCaptured(class AFlag* Flag, class AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team;
	if (bValidCapture)
	{
		ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
		if (BGameState)
		{
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
	return Super::CalculateDamage(Attacker, Victim, BaseDamage);
}
