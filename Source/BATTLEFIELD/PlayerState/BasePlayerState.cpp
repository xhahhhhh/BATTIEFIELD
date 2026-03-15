// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerState.h"

#include "../Character/BaseCharacter.h"
#include "../PlayerController/BasePlayerController.h"
#include "Net/UnrealNetwork.h"

void ABasePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 配置需要网络复制的属性
	// 这些属性会自动从服务器同步到所有客户端
	DOREPLIFETIME(ABasePlayerState, Defeats);  // 死亡次数
	DOREPLIFETIME(ABasePlayerState, Team);     // 所属队伍
}

void ABasePlayerState::OnRep_Score()
{
	// 调用父类实现
	Super::OnRep_Score();

	//============================
	// 更新HUD分数显示
	//============================
	
	// 获取或缓存角色引用
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		// 获取或缓存控制器引用
		Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			// 更新HUD上的分数显示
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABasePlayerState::OnRep_Defeats()
{
	//============================
	// 更新HUD死亡次数显示
	//============================
	
	// 获取或缓存角色引用
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		// 获取或缓存控制器引用
		Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			// 更新HUD上的死亡次数显示
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABasePlayerState::AddToScore(float ScoreAmount)
{
	//============================
	// 增加分数（服务器执行）
	//============================
	
	// 使用SetScore确保网络复制触发
	SetScore(GetScore() + ScoreAmount);

	// 立即更新服务器上的HUD显示
	// 客户端通过OnRep_Score回调更新
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABasePlayerState::AddToDefeats(int32 DefeatsAmount)
{
	//============================
	// 增加死亡次数（服务器执行）
	//============================
	
	Defeats += DefeatsAmount;

	// 立即更新服务器上的HUD显示
	// 客户端通过OnRep_Defeats回调更新
	Character = Character == nullptr ? Cast<ABaseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABasePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABasePlayerState::SetTeam(ETeam TeamToSet)
{
	//============================
	// 设置队伍（服务器执行）
	//============================
	
	Team = TeamToSet;

	// 立即在服务器上更新角色颜色
	// 客户端通过OnRep_Team回调更新
	ABaseCharacter* BCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BCharacter)
	{
		BCharacter->SetTeamColor(Team);
	}
}

void ABasePlayerState::OnRep_Team()
{
	//============================
	// 队伍变更回调（客户端执行）
	//============================
	
	// 更新角色的队伍颜色显示
	ABaseCharacter* BCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BCharacter)
	{
		BCharacter->SetTeamColor(Team);
	}
}
