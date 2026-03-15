// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerGameMode.h"
#include "../Character/BaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "../PlayerController/BasePlayerController.h"
#include "../PlayerState/BasePlayerState.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "GameFramework/PlayerStart.h"

// 定义冷却状态的常量
const FName MatchState::Cooldown = FName("Cooldown");

APlayerGameMode::APlayerGameMode()
{
	// 启用延迟开始，等待所有玩家准备好再开始匹配
	bDelayedStart = true;
}

void APlayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 记录关卡开始时间，用于计算倒计时
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void APlayerGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//============================
	// 等待开始阶段：热身倒计时
	//============================
	if (MatchState == MatchState::WaitingToStart)
	{
		// 计算剩余热身时间
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// 热身结束，正式开始匹配
			StartMatch();
		}
	}
	//============================
	// 进行中阶段：比赛倒计时
	//============================
	else if (MatchState == MatchState::InProgress)
	{
		// 计算剩余比赛时间
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// 比赛结束，进入冷却阶段
			SetMatchState(MatchState::Cooldown);
		}
	}
	//============================
	// 冷却阶段：等待下一句
	//============================
	else if (MatchState == MatchState::Cooldown)
	{
		// 计算剩余冷却时间
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// 冷却结束，重新开始游戏
			RestartGame();
		}
	}
}

void APlayerGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// 通知所有玩家控制器匹配状态变化
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABasePlayerController* BasePlayer = Cast<ABasePlayerController>(*It);
		if (BasePlayer)
		{
			BasePlayer->OnMatchStateSet(MatchState, bTeamMatch);
		}
	}
}

float APlayerGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	// 基础实现：直接返回基础伤害
	// 子类可重写实现友军伤害等逻辑
	return BaseDamage;
}

void APlayerGameMode::PlayerEliminated(class ABaseCharacter* EliminatedCharacter,
                                       ABasePlayerController* VictimController,
                                       ABasePlayerController* AttackerController)
{
	// 获取攻击者和受害者的玩家状态
	ABasePlayerState* AttackerPlayerState = AttackerController
		                                        ? Cast<ABasePlayerState>(AttackerController->PlayerState)
		                                        : nullptr;
	ABasePlayerState* VictimPlayerState = VictimController
	                                      ? Cast<ABasePlayerState>(VictimController->PlayerState)
	                                      : nullptr;

	// 获取游戏状态
	ABaseGameState* BaseGameState = GetGameState<ABaseGameState>();

	//============================
	// 更新攻击者分数和排行榜
	//============================
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BaseGameState)
	{
		// 记录当前领先玩家列表
		TArray<ABasePlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : BaseGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.AddUnique(LeadPlayer);
		}

		// 为攻击者加分
		AttackerPlayerState->AddToScore(1.f);
		BaseGameState->UpdateTopScore(AttackerPlayerState);

		// 检查攻击者是否成为新的领先者
		if (BaseGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ABaseCharacter* Leader = Cast<ABaseCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				// 多播通知该玩家获得领先
				Leader->MulticastGainedTheLead();
			}
		}

		// 检查是否有之前的领先者失去领先地位
		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!BaseGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABaseCharacter* Loser = Cast<ABaseCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					// 多播通知该玩家失去领先
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	//============================
	// 更新受害者死亡数
	//============================
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	//============================
	// 执行角色淘汰
	//============================
	if (EliminatedCharacter)
	{
		// false表示正常淘汰（非离开游戏）
		EliminatedCharacter->Elim(false);
	}

	//============================
	// 广播击杀信息给所有玩家
	//============================
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABasePlayerController* BasePlayerController = Cast<ABasePlayerController>(*It);
		if (BasePlayerController && AttackerPlayerState && VictimPlayerState)
		{
			BasePlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void APlayerGameMode::RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	// 销毁被消灭的角色
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}

	// 在随机出生点重生控制器
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void APlayerGameMode::PlayerLeftGame(ABasePlayerState* PlayerLeaving)
{
	// 安全检查
	if (PlayerLeaving == nullptr) return;

	// 从排行榜中移除离开的玩家
	ABaseGameState* BaseGameState = GetGameState<ABaseGameState>();
	if (BaseGameState && BaseGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BaseGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}

	// 消灭该玩家的角色（true表示因离开游戏而淘汰）
	ABaseCharacter* CharacterLeaving = Cast<ABaseCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}
