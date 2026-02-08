
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "BATTLEFIELD/CharacterTypes/Team.h"
#include "TeamPlayerStart.generated.h"

UCLASS()
class BATTLEFIELD_API ATeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	ETeam Team;
};
