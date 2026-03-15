// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "BATTLEFIELD/PlayerState/BasePlayerState.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	// 标记为团队模式，影响UI显示等
	bTeamMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 获取游戏状态以访问队伍列表
	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		// 获取新玩家的玩家状态
		ABasePlayerState* BPState = NewPlayer->GetPlayerState<ABasePlayerState>();
		// 只为未分配队伍的玩家分配
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
		{
			// 平衡分配：将玩家分配到人数较少的队伍
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_RedTeam);
			}
			else
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 获取游戏状态和退出玩家的玩家状态
	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	ABasePlayerState* BPState = Exiting->GetPlayerState<ABasePlayerState>();
	if (BGameState && BPState)
	{
		// 从相应队伍中移除
		if (BGameState->RedTeam.Contains(BPState))
		{
			BGameState->RedTeam.Remove(BPState);
		}
		else if (BGameState->BlueTeam.Contains(BPState))
		{
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// 获取游戏状态
	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		// 遍历所有玩家，为尚未分配队伍的自动分配
		for (auto PState : BGameState->PlayerArray)
		{
			ABasePlayerState* BPState = Cast<ABasePlayerState>(PState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
			{
				// 平衡分配
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	// 获取双方的玩家状态
	ABasePlayerState* AttackerPState = Attacker->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* VictimPState = Victim->GetPlayerState<ABasePlayerState>();

	// 安全检查
	if (AttackerPState == nullptr || VictimPState == nullptr) return BaseDamage;

	// 自杀：自己对自己不造成伤害
	if (VictimPState == AttackerPState) return 0.f;

	// 友军伤害免疫：队友之间不造成伤害
	if (AttackerPState->GetTeam() == VictimPState->GetTeam()) return 0.f;

	// 非队友造成全额伤害
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
	ABasePlayerController* VictimController, ABasePlayerController* AttackerController)
{
	// 调用父类实现（更新个人分数等）
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);

	// 获取游戏状态和攻击者玩家状态
	ABaseGameState* BGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
	ABasePlayerState* AttackerPlayerState = AttackerController ? 
		Cast<ABasePlayerState>(AttackerController->PlayerState) : nullptr;

	// 为攻击者所属队伍增加团队分数
	if (BGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BGameState->BlueTeamScores();
		}
		if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BGameState->RedTeamScores();
		}
	}
}
