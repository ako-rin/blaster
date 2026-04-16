// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "BlasterComponent/CombatComponent.h"
#include "Components/ProgressBar.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerState.h"
#include "Components/Image.h"
#include "Net/UnrealNetwork.h"
#include "Gamemode/BlasterGameMode.h"
#include "HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "GameState/BlasterGameState.h"
#include "Weapon/Weapon.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 仅在时间更改时设置 HUD 时间
	SetHUDTime();

	// 每 5s 同步一次服务器时间
	CheckTimeSync(DeltaSeconds);

	// 在角色 BeginPlay 时 HUD 还未准备好 
	PollInit();
	
	CheckPing(DeltaSeconds);
	
	// 更新手雷时间
	if (bCheckGrenadeCooldown)
	{
		UpdateHUDGrenadeCooldown();
	}
	
	if (HasAuthority()) 
	{
		BlasterHUD = Cast<ABlasterHUD>(GetHUD());
		if (BlasterHUD)
		{
			// 如果处于等待状态，但公告板没建出来，强行建！
			if (MatchState == MatchState::WaitingToStart && BlasterHUD->GetAnnouncement() == nullptr)
			{
				BlasterHUD->AddAnnouncement();
			}
			// 如果处于游戏中，但血条面板没建出来，强行建！
			else if (MatchState == MatchState::InProgress && BlasterHUD->GetCharacterOverlay() == nullptr)
			{
				HandleMatchHasStarted();
			}
		}
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
	
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// if DedicatedServer, will return false
	if (!IsLocalController())
		return;
	AddDefaultActions();
}

// Only On Server
void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (!InPawn) return;

	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		
		if (BlasterCharacter->GetCombat())
		{
			HUDMaxGrenadeCooldownTime = BlasterCharacter->GetCombat()->GetGrenadeMaxCooldownTime();
			SetHUDGrenadeCooldown(HUDMaxGrenadeCooldownTime, HUDMaxGrenadeCooldownTime);
			
			if (BlasterCharacter->GetCombat()->EquippedWeapon)
			{
				SetHUDAmmo(BlasterCharacter->GetCombat()->EquippedWeapon->GetCurrentAmmo());
				SetHUDCarriedAmmo(BlasterCharacter->GetCombat()->GetCarriedAmmoFromAmmoMap());
			}
		}
		if (ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>())
		{
			SetHUDScore(BlasterPlayerState->GetScore());
			SetHUDDefeats(BlasterPlayerState->GetDefeats());
		}

		if (IsLocalController())
		{
			AddDefaultActions();
		}
	}
}

void ABlasterPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (IsLocalController())
	{
		AddDefaultActions();
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
		&& BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HealthBar
		&& BlasterHUD->GetCharacterOverlay()->HealthText;

	if (bHUDValid) 
	{
		bInitializeHealth = false;
		
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->GetCharacterOverlay()->HealthBar->SetPercent(HealthPercent);
		
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
		
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
		&& BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->ShieldBar
		&& BlasterHUD->GetCharacterOverlay()->ShieldText;

	if (bHUDValid) 
	{
		bInitializeShield = false;
		
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->GetCharacterOverlay()->ShieldBar->SetPercent(ShieldPercent);
		
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->GetCharacterOverlay()->ShieldText->SetText(FText::FromString(ShieldText));
		
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetCharacterOverlay()
	&& BlasterHUD->GetCharacterOverlay()->ScoreText
	&& BlasterHUD->GetCharacterOverlay()->ScoreAmount;

	if (bHUDValid)
	{
		bInitializeScore = false;
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->GetCharacterOverlay()->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetCharacterOverlay()
	&& BlasterHUD->GetCharacterOverlay()->DefeatsText
	&& BlasterHUD->GetCharacterOverlay()->DefeatsAmount;

	if (bHUDValid)
	{
		bInitializeDefeats = false;
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->GetCharacterOverlay()->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetCharacterOverlay()
	&& BlasterHUD->GetCharacterOverlay()->AmmoText
	&& BlasterHUD->GetCharacterOverlay()->AmmoAmount;

	if (bHUDValid)
	{
		bInitializeAmmo = false;
		const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeAmmo = true;
		HUDAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetCharacterOverlay()
	&& BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount;

	if (bHUDValid)
	{
		bInitializeCarriedAmmo = false;
		const FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetCharacterOverlay()
	&& BlasterHUD->GetCharacterOverlay()->MatchCountdownText;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = FMath::FloorToInt(CountdownTime - Minutes * 60.f); // 向下取整
		
		const FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetCharacterOverlay()->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
	&& BlasterHUD->GetAnnouncement()
	&& BlasterHUD->GetAnnouncement()->WarmupTime;

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = FMath::FloorToInt(CountdownTime - Minutes * 60.f); // 向下取整
		
		const FString CountdownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);
		BlasterHUD->GetAnnouncement()->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenadeCooldown(float ThrowCooldownTime, float ThrowMaxCooldownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
		&& BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->GrenadeProgressBar;

	if (bHUDValid) 
	{
		bInitializeGrenade = false;
		
		const float CooldownPercent = ThrowCooldownTime / ThrowMaxCooldownTime;
		BlasterHUD->GetCharacterOverlay()->GrenadeProgressBar->SetPercent(CooldownPercent);
	}
	else
	{
		bInitializeGrenade = true;
		HUDGrenadeCooldownTime = ThrowCooldownTime;
		HUDMaxGrenadeCooldownTime = ThrowMaxCooldownTime;
	}
}

void ABlasterPlayerController::UpdateHUDGrenadeCooldown()
{
	float ElapsedTime = GetServerTime() - HUDGrenadeStartTime;
	if (ElapsedTime >= HUDMaxGrenadeCooldownTime)
	{
		bCheckGrenadeCooldown = false;
		SetHUDGrenadeCooldown(HUDMaxGrenadeCooldownTime, HUDMaxGrenadeCooldownTime);
	}
	else
	{
		SetHUDGrenadeCooldown(ElapsedTime, HUDMaxGrenadeCooldownTime);
	}
}

void ABlasterPlayerController::StartGrenadeCooldown(float StartTime)
{
	HUDGrenadeStartTime = StartTime;
	bCheckGrenadeCooldown = true;
}

// Tick
void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft); // 向上取整

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
                          		{
			SetHUDMatchCountdown(TimeLeft);
		}
		
	}
	CountdownInt = SecondsLeft;
	
}

// client join
void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (HasAuthority() && MatchState == MatchState::WaitingToStart)
	{
		if (ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			WarmupTime = GameMode->GetWarmupTime();
			MatchTime = GameMode->GetMatchTime();
			LevelStartingTime = GameMode->GetLevelStartingTime();
			CooldownTime = GameMode->GetCooldownTime();
		}
	}
	
	if (MatchState == MatchState::WaitingToStart)
	{
		
	}

	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
		&& BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingIcon
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;

	if (bHUDValid) 
	{
		BlasterHUD->GetCharacterOverlay()->HighPingIcon->SetOpacity(1.f);
		BlasterHUD->GetCharacterOverlay()->PlayAnimation(
			BlasterHUD->GetCharacterOverlay()->HighPingAnimation,
			0.f,
			5
		);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD
		&& BlasterHUD->GetCharacterOverlay()
		&& BlasterHUD->GetCharacterOverlay()->HighPingIcon
		&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation;

	if (bHUDValid) 
	{
		BlasterHUD->GetCharacterOverlay()->HighPingIcon->SetOpacity(0.f);
		if (BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation))
		{
			BlasterHUD->GetCharacterOverlay()->StopAnimation((BlasterHUD->GetCharacterOverlay()->HighPingAnimation));
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? TObjectPtr<APlayerState>(GetPlayerState<APlayerState>()) : PlayerState;
		if (PlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("Ping :%d"), (PlayerState->GetCompressedPing() << 2))
			if ((PlayerState->GetCompressedPing() << 2) > HighPingThreshold)
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
	
	bool bHighPingAnimationPlaying = BlasterHUD 
			&& BlasterHUD->GetCharacterOverlay() 
			&& BlasterHUD->GetCharacterOverlay()->HighPingAnimation 
			&& BlasterHUD->GetCharacterOverlay()->IsAnimationPlaying(BlasterHUD->GetCharacterOverlay()->HighPingAnimation);
	if (bHighPingAnimationPlaying)	
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
			PingAnimationRunningTime = 0.f;
		}
	}
}

// Is the pign too high
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}


void ABlasterPlayerController::HandleMatchHasStarted()
{
	// BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->GetAnnouncement())
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	// BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD)
	{
		if (BlasterHUD->GetCharacterOverlay())
		{
			BlasterHUD->GetCharacterOverlay()->RemoveFromParent();
		}
		bool bHUDValid = BlasterHUD->GetAnnouncement() &&
			BlasterHUD->GetAnnouncement()->AnnouncementText &&
				BlasterHUD->GetAnnouncement()->InfoText;
		if (bHUDValid)
		{
			BlasterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Start In:");
			BlasterHUD->GetAnnouncement()->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterPlayerState)
			{
				if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
				{
					TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
					FString InfoTextString;					
					if (TopPlayers.Num() == 0)
					{
						InfoTextString = FString("There is no winner.");
					}
					else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
					{
						// This controller of character is winner
						InfoTextString = FString("You are the winner!");
					}
					else if (TopPlayers.Num() == 1) // other
					{
						// This controller of character is not winner
						InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
					}
					else if (TopPlayers.Num() > 1) // local
					{
						InfoTextString = FString("Players tied for the win!\n");
						for (const auto& TiedPlayer : TopPlayers)
						{
							InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
						}
					}
					BlasterHUD->GetAnnouncement()->InfoText->SetText(FText::FromString(InfoTextString));
				}
			}
		}
	}

	RemoveDefaultActions();
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->SetDisableCharacterGameplay(true);
	}
}

void ABlasterPlayerController::PollInit()
{
	if (BlasterHUD && BlasterHUD->GetCharacterOverlay())
	{
		CharacterOverlay = BlasterHUD->GetCharacterOverlay();
		if (CharacterOverlay)
		{
			if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
			if (bInitializeScore)	SetHUDScore(HUDScore);
			if (bInitializeDefeats)	SetHUDDefeats(HUDDefeats);
			if (bInitializeAmmo)	SetHUDAmmo(HUDAmmo);
			if (bInitializeCarriedAmmo)	SetHUDCarriedAmmo(HUDCarriedAmmo);
			if (bInitializeGrenade) SetHUDGrenadeCooldown(HUDGrenadeCooldownTime, HUDMaxGrenadeCooldownTime);
			if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
		}
	}
}

void ABlasterPlayerController::RemoveDefaultActions()
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn()))
	{
		BlasterCharacter->FireReleased();
		BlasterCharacter->AimReleased();
		BlasterCharacter->CrouchReleased();
		BlasterCharacter->WalkReleased();
	}
	InputSubsystem = InputSubsystem == nullptr ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : InputSubsystem;
	if (InputSubsystem)
	{
		InputSubsystem->RemoveMappingContext(DefaultMappingContexts);
	}
}

void ABlasterPlayerController::AddDefaultActions()
{
	InputSubsystem = InputSubsystem == nullptr ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : InputSubsystem;
	if (InputSubsystem)
	{
		InputSubsystem->ClearAllMappings();

		if (DefaultMappingContexts)
		{
			InputSubsystem->AddMappingContext(DefaultMappingContexts, 0);			
		}
		if (MouseLookMappingContexts)
		{
			InputSubsystem->AddMappingContext(MouseLookMappingContexts, 1);
		}
	}
}

// begin play
void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if (ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		WarmupTime = GameMode->GetWarmupTime();
		MatchTime = GameMode->GetMatchTime();
		LevelStartingTime = GameMode->GetLevelStartingTime();
		CooldownTime = GameMode->GetCooldownTime();
		MatchState = GameMode->GetMatchState();

		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		{
			// Owning Connection is self ,Client will call this function ↓
			// BlasterHUD->AddAnnouncement();
		}
		else if (BlasterHUD && MatchState == MatchState::Cooldown)
		{
			// Owning Connection is self ,Client will call this function ↓
			// InputSubsystem = InputSubsystem == nullptr ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : InputSubsystem;
			// if (InputSubsystem)
			// {
			// 	InputSubsystem->RemoveMappingContext(DefaultMappingContexts);
			// }
		}
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	
	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}

	else if (BlasterHUD && MatchState == MatchState::InProgress)
	{
		
	}

	else if (BlasterHUD && MatchState == MatchState::Cooldown)
	{
		InputSubsystem = InputSubsystem == nullptr ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()) : InputSubsystem;
		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(DefaultMappingContexts);
		}
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TImeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TImeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}


