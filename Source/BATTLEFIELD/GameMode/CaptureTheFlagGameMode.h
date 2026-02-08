
#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagGameMode.generated.h"


UCLASS()
class BATTLEFIELD_API ACaptureTheFlagGameMode : public ATeamsGameMode
{
	GENERATED_BODY()
public:
	virtual void PlayerEliminated(class ABaseCharacter* EliminatedCharacter,ABasePlayerController* VictimController,ABasePlayerController* AttackerController)override;
	void FlagCaptured(class AFlag* Flag,class AFlagZone* Zone);
};
