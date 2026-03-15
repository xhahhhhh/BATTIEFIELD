// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "../HUD/PlayerHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "../Character/BaseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "../GameMode/PlayerGameMode.h"
#include "BATTLEFIELD/Components/CombatComponent.h"
#include "BATTLEFIELD/HUD/Announcement.h"
#include "BATTLEFIELD/GameState/BaseGameState.h"
#include "BATTLEFIELD/HUD/PauseMenu.h"
#include "BATTLEFIELD/PlayerState/BasePlayerState.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "BATTLEFIELD/CharacterTypes/Announcement.h"

//============================
// 击杀公告
//============================

void ABasePlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	// 服务器调用客户端RPC显示击杀公告
	ClientElimAnnouncement(Attacker, Victim);
}

void ABasePlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	// 获取自身玩家状态
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
		if (PlayerHUD)
		{
			// 根据玩家关系显示不同的击杀信息
			
			// 自己击杀其他玩家
			if (Attacker == Self && Victim != Self)
			{
				PlayerHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			// 自己被其他玩家击杀
			if (Victim == Self && Attacker != Self)
			{
				PlayerHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			// 自己击杀自己（自杀）
			if (Attacker == Victim && Attacker == Self)
			{
				PlayerHUD->AddElimAnnouncement("You", "Yourself");
				return;
			}
			// 其他玩家自杀
			if (Attacker == Victim && Attacker != Self)
			{
				PlayerHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Attacker->GetPlayerName());
				return;
			}
			// 其他玩家之间的击杀
			PlayerHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

//============================
// 生命周期
//============================

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 缓存HUD引用
	PlayerHUD = Cast<APlayerHUD>(GetHUD());
	
	// 请求服务器同步匹配状态
	ServerCheckMatchState();
}

void ABasePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 配置网络复制属性
	DOREPLIFETIME(ABasePlayerController, MatchState);
	DOREPLIFETIME(ABasePlayerController, bShowTeamScores);
}

//============================
// 团队分数
//============================

void ABasePlayerController::HideTeamScores()
{
	// 隐藏团队分数显示
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->RedTeamScore && 
		PlayerHUD->CharacterOverlay->BlueTeamScore && 
		PlayerHUD->CharacterOverlay->ScoreSpacerText;
	
	if (bHUDValid)
	{
		PlayerHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		PlayerHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		PlayerHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void ABasePlayerController::InitTeamScores()
{
	// 初始化团队分数显示为0
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->RedTeamScore && 
		PlayerHUD->CharacterOverlay->BlueTeamScore && 
		PlayerHUD->CharacterOverlay->ScoreSpacerText;
	
	if (bHUDValid)
	{
		FString Zero("0");
		FString Spacer(":");
		PlayerHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		PlayerHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		PlayerHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void ABasePlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	// 设置红队分数显示
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->RedTeamScore;
	
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		PlayerHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABasePlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	// 设置蓝队分数显示
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->BlueTeamScore;
	
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		PlayerHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 控制新角色时设置初始血量显示
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(InPawn);
	if (BaseCharacter)
	{
		SetHUDHealth(BaseCharacter->GetHealth(), BaseCharacter->GetMaxHealth());
	}
}

//============================
// Tick更新
//============================

void ABasePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每帧更新HUD时间显示
	SetHUDTime();
	
	// 检查是否需要时间同步
	CheckTimeSync(DeltaTime);
	
	// 轮询初始化缓存的HUD值
	PollInit();
	
	// 检查网络延迟
	CheckPing(DeltaTime);
}

void ABasePlayerController::CheckPing(float DeltaTime)
{
	// 累计检测时间
	HighPingRunningTime += DeltaTime;
	
	if (HighPingRunningTime > CheckPingFrequency)
	{
		// 获取玩家状态检查延迟
		PlayerState = PlayerState == nullptr ? GetPlayerState<ABasePlayerState>() : PlayerState;
		if (PlayerState)
		{
			// 延迟超过阈值
			if (PlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	// 检查高延迟动画是否正在播放
	bool bHighPingAnimationPlaying = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->HighPingAnimation && 
		PlayerHUD->CharacterOverlay->IsAnimationPlaying(PlayerHUD->CharacterOverlay->HighPingAnimation);
	
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		// 动画播放超过持续时间后停止
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

//============================
// 暂停菜单
//============================

void ABasePlayerController::ShowReturnToMainMenu()
{
	if (PauseMenuWidget == nullptr) return;
	
	if (PauseMenu == nullptr)
	{
		PauseMenu = CreateWidget<UPauseMenu>(this, PauseMenuWidget);
	}
	
	if (PauseMenu)
	{
		// 切换暂停菜单状态
		bPauseMenuOpen = !bPauseMenuOpen;
		if (bPauseMenuOpen)
		{
			PauseMenu->MenuSetup();
		}
		else
		{
			PauseMenu->MenuTearDown();
		}
	}
}

//============================
// 团队分数复制回调
//============================

void ABasePlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

//============================
// 时间同步
//============================

float ABasePlayerController::GetServerTime()
{
	// 服务器直接返回世界时间
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	// 客户端返回世界时间加时间差
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABasePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	// 本地控制器开始时间同步
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (InputComponent == nullptr) return;
	
	// 添加输入映射上下文
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ControllerMappingContext)
		{
			Subsystem->AddMappingContext(ControllerMappingContext, 1);
		}
	}
	
	// 绑定退出/暂停操作
	if (UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(IA_Quit, ETriggerEvent::Started, this,
		                                   &ABasePlayerController::ShowReturnToMainMenu);
	}
}

//============================
// HUD更新函数
//============================

void ABasePlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->HealthBar && 
		PlayerHUD->CharacterOverlay->HealthText;
	
	if (bHUDValid)
	{
		// 更新血量条和文本
		const float HealthPercent = Health / MaxHealth;
		PlayerHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), 
			FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		PlayerHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		// HUD未准备好，缓存数值
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABasePlayerController::SetHUDShield(float Shield, float MaxShield)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->ShieldBar && 
		PlayerHUD->CharacterOverlay->ShieldText;
	
	if (bHUDValid)
	{
		// 更新护盾条和文本
		const float ShieldPercent = Shield / MaxShield;
		PlayerHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d / %d"), 
			FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		PlayerHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABasePlayerController::SetHUDScore(float Score)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->ScoreAmount;
	
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABasePlayerController::SetHUDDefeats(int32 Defeats)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->DefeatsAmount;
	
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		PlayerHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABasePlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->WeaponAmmoAmount;
	
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PlayerHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABasePlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->CarriedAmmoAmount;
	
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PlayerHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABasePlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->MatchCountdownText;
	
	if (bHUDValid)
	{
		if (CountdownTime < 0.0f)
		{
			PlayerHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		// 格式化为 MM:SS
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - (Minutes * 60.f);
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PlayerHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABasePlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->Announcement && 
		PlayerHUD->Announcement->WarmupTime;
	
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			PlayerHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		// 格式化为 MM:SS
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - (Minutes * 60.f);
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PlayerHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABasePlayerController::SetHUDGrenades(int32 Grenades)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->GrenadesText;
	
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		PlayerHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

//============================
// 时间显示管理
//============================

void ABasePlayerController::SetHUDTime()
{
	// 根据匹配状态计算剩余时间
	float TimeLeft = 0.f;
	
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartTime;
	}

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	// 服务器直接从游戏模式获取倒计时
	if (HasAuthority())
	{
		PlayerGameMode = PlayerGameMode == nullptr
			                 ? Cast<APlayerGameMode>(UGameplayStatics::GetGameMode(this))
			                 : PlayerGameMode;
		if (PlayerGameMode)
		{
			LevelStartTime = PlayerGameMode->LevelStartingTime;
			SecondsLeft = FMath::CeilToInt(PlayerGameMode->GetCountdownTime() + LevelStartTime);
		}
	}

	// 每秒更新一次显示
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(SecondsLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(SecondsLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

//============================
// 轮询初始化
//============================

void ABasePlayerController::PollInit()
{
	// HUD可能延迟加载，轮询检查并应用缓存值
	if (CharacterOverlay == nullptr)
	{
		if (PlayerHUD && PlayerHUD->CharacterOverlay)
		{
			this->CharacterOverlay = PlayerHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				// 应用缓存的初始化值
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn());
				if (BaseCharacter && BaseCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(BaseCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ABasePlayerController::CheckTimeSync(float DeltaTime)
{
	// 累计同步时间
	TimeSyncRunningTime += DeltaTime;
	
	// 本地控制器定期请求时间同步
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

//============================
// 高延迟警告
//============================

void ABasePlayerController::HighPingWarning()
{
	// 显示高延迟警告动画
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->HighPingImage && 
		PlayerHUD->CharacterOverlay->HighPingAnimation;
	
	if (bHUDValid)
	{
		PlayerHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		PlayerHUD->CharacterOverlay->PlayAnimation(
			PlayerHUD->CharacterOverlay->HighPingAnimation, 0.f, 5);
	}
}

void ABasePlayerController::StopHighPingWarning()
{
	// 停止高延迟警告动画
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->CharacterOverlay && 
		PlayerHUD->CharacterOverlay->HighPingImage && 
		PlayerHUD->CharacterOverlay->HighPingAnimation;
	
	if (bHUDValid)
	{
		PlayerHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (PlayerHUD->CharacterOverlay->IsAnimationPlaying(
			PlayerHUD->CharacterOverlay->HighPingAnimation))
		{
			PlayerHUD->CharacterOverlay->StopAnimation(
				PlayerHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

//============================
// 匹配状态同步RPC
//============================

void ABasePlayerController::ServerCheckMatchState_Implementation()
{
	// 服务器返回当前匹配状态给客户端
	APlayerGameMode* GameMode = Cast<APlayerGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		LevelStartTime = GameMode->LevelStartingTime;
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		MatchState = GameMode->GetMatchState();
		CooldownTime = GameMode->CooldownTime;
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartTime);
	}
}

void ABasePlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup,
                                                             float Match, float Cooldown, float StartingTime)
{
	// 客户端接收匹配状态并初始化
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	MatchState = StateOfMatch;
	LevelStartTime = StartingTime;
	OnMatchStateSet(MatchState);

	// 热身阶段显示公告
	if (PlayerHUD && MatchState == MatchState::WaitingToStart)
	{
		PlayerHUD->AddAnnouncement();
	}
}

//============================
// 时间同步RPC
//============================

void ABasePlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	// 服务器记录接收时间并返回给客户端
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABasePlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                  float TimeServerReceivedClientRequest)
{
	// 客户端计算往返时间和单程延迟
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	
	// 计算服务器当前时间和客户端时间差
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

//============================
// 匹配状态处理
//============================

void ABasePlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABasePlayerController::OnRep_MatchState()
{
	// 匹配状态复制回调
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABasePlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	// 服务器接收客户端延迟报告并广播
	HighPingDelegate.Broadcast(bHighPing);
}

void ABasePlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	// 服务器设置团队分数显示标志
	if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		// 显示角色状态面板
		if (PlayerHUD->CharacterOverlay == nullptr)
		{
			PlayerHUD->AddCharacterOverlay();
		}
		
		// 隐藏公告
		if (PlayerHUD->Announcement)
		{
			PlayerHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		
		// 非服务器端不执行后续操作
		if (!HasAuthority()) return;
		
		// 初始化或隐藏团队分数
		if (bTeamsMatch)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void ABasePlayerController::HandleCooldown()
{
	// 处理冷却阶段显示
	PlayerHUD = PlayerHUD == nullptr ? Cast<APlayerHUD>(GetHUD()) : PlayerHUD;
	if (PlayerHUD)
	{
		// 移除角色面板
		PlayerHUD->CharacterOverlay->RemoveFromParent();
		
		// 显示公告和比赛结果
		bool bHUDValid = PlayerHUD->Announcement && 
			PlayerHUD->Announcement->AnnouncementText && 
			PlayerHUD->Announcement->InfoText;
		
		if (bHUDValid)
		{
			PlayerHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			PlayerHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			// 获取获胜信息
			ABaseGameState* BaseGameState = Cast<ABaseGameState>(UGameplayStatics::GetGameState(this));
			ABasePlayerState* BasePlayerState = GetPlayerState<ABasePlayerState>();
			if (BaseGameState && BasePlayerState)
			{
				TArray<ABasePlayerState*> TopPlayers = BaseGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? 
					GetTeamsInfoText(BaseGameState) : GetInfoText(TopPlayers);
				PlayerHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	
	// 禁用角色输入
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetPawn());
	if (BaseCharacter && BaseCharacter->GetCombat())
	{
		BaseCharacter->bDisableGameplay = true;
		BaseCharacter->GetCombat()->ShootPressed(false);
	}
}

//============================
// 比赛结果信息
//============================

FString ABasePlayerController::GetInfoText(const TArray<ABasePlayerState*>& Players)
{
	// 生成混战模式结束信息
	ABasePlayerState* BasePlayerState = GetPlayerState<ABasePlayerState>();
	if (BasePlayerState == nullptr) return FString();
	
	FString InfoTextString;
	
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == BasePlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1)
	{
		InfoTextString = FString::Printf(TEXT("获胜者:\n %s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString("\n"));
		for (auto TopPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s \n"), *TopPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ABasePlayerController::GetTeamsInfoText(ABaseGameState* BaseGameState)
{
	// 生成团队模式结束信息
	if (BaseGameState == nullptr) return FString();
	
	FString InfoTextString;
	const int32 RedTeamScore = BaseGameState->RedTeamScore;
	const int32 BlueTeamScore = BaseGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));
	}
	return InfoTextString;
}
