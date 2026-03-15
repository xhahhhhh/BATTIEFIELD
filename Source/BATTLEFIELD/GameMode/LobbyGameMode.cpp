// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiPlayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 启用无缝旅行，确保玩家在大厅和游戏地图之间平滑过渡
	// 避免重新连接时的加载屏幕
	bUseSeamlessTravel = true;

	// 获取当前玩家数量
	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	// 获取游戏实例中的多人会话子系统
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiPlayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiPlayerSessionsSubsystem>();
		check(Subsystem);

		// 检查是否达到设定的玩家数
		if (NumberOfPlayers == Subsystem->GetDesiredNumPublicConnections())
		{
			UWorld* World = GetWorld();
			if (World)
			{
				// 根据匹配类型切换到相应地图
				// 目前所有模式使用同一张地图，但可以通过不同参数区分
				FString MatchType = Subsystem->GetDesiredMatchType();

				if (MatchType == "FreeForAll")
				{
					// 混战模式：切换到游戏地图
					World->ServerTravel(FString("/Game/Maps/Dessert?listen"));
				}
				else if (MatchType == "Teams")
				{
					// 团队模式：切换到游戏地图
					World->ServerTravel(FString("/Game/Maps/Dessert?listen"));
				}
				else if (MatchType == "CaptureTheFlag")
				{
					// 夺旗模式：切换到游戏地图
					World->ServerTravel(FString("/Game/Maps/Dessert?listen"));
				}
			}
		}
	}
}
