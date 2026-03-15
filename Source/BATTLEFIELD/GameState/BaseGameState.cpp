// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameState.h"

#include "BATTLEFIELD/PlayerController/BasePlayerController.h"
#include "BATTLEFIELD/PlayerState/BasePlayerState.h"
#include "Net/UnrealNetwork.h"

void ABaseGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 配置需要网络复制的属性
	// 这些属性会自动从服务器同步到所有客户端
	DOREPLIFETIME(ABaseGameState, TopScoringPlayers);  // 排行榜列表
	DOREPLIFETIME(ABaseGameState, RedTeamScore);       // 红队分数
	DOREPLIFETIME(ABaseGameState, BlueTeamScore);      // 蓝队分数
}

void ABaseGameState::UpdateTopScore(ABasePlayerState* ScoringPlayer)
{
	//============================
	// 情况1：排行榜为空，该玩家成为第一
	//============================
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	//============================
	// 情况2：分数等于当前最高分，加入并列第一
	//============================
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		// AddUnique 确保同一玩家不会重复添加
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	//============================
	// 情况3：分数超过当前最高分，成为新的唯一第一
	//============================
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		// 清空之前的并列第一列表
		TopScoringPlayers.Empty();
		// 该玩家成为新的第一
		TopScoringPlayers.AddUnique(ScoringPlayer);
		// 更新最高分数记录
		TopScore = ScoringPlayer->GetScore();
	}
	// 情况4：分数低于最高分，不做任何处理
}

void ABaseGameState::RedTeamScores()
{
	// 增加红队分数（仅在服务器执行）
	++RedTeamScore;

	// 在服务器上直接更新本地玩家的HUD
	// 注意：其他客户端通过 OnRep_RedTeamScore 回调更新
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABaseGameState::BlueTeamScores()
{
	// 增加蓝队分数（仅在服务器执行）
	++BlueTeamScore;

	// 在服务器上直接更新本地玩家的HUD
	// 注意：其他客户端通过 OnRep_BlueTeamScore 回调更新
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}

void ABaseGameState::OnRep_RedTeamScore()
{
	// 当 RedTeamScore 从服务器复制到客户端时自动调用
	// 更新客户端的HUD显示
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDRedTeamScore(RedTeamScore);
	}
}

void ABaseGameState::OnRep_BlueTeamScore()
{
	// 当 BlueTeamScore 从服务器复制到客户端时自动调用
	// 更新客户端的HUD显示
	ABasePlayerController* BPlayer = Cast<ABasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (BPlayer)
	{
		BPlayer->SetHUDBlueTeamScore(BlueTeamScore);
	}
}
