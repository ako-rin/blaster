// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterType/Team.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

class UInputMappingContext;

/**
 * Player Controller for the Third Person Game
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void ReceivedPlayer() override; // 最早可获取时间 Sync with server clock as soon as possible
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Pawn() override;
	
	UFUNCTION()
	void OnRep_ShowTeamScores();
	
protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;


public:
	void ShowReturnToMainMenu();
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim);
	
	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(APlayerState* Attacker, APlayerState* Victim);
	
	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);
	
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDGrenadeCooldown(float ThrowCooldownTime, float ThrowMaxCooldownTime);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDAnnouncementCountdown(float CountdownTime);

	void UpdateHUDGrenadeCooldown();
	void StartGrenadeCooldown(float StartTime);

	// 处理玩家加入游戏时，需要处理的逻辑
	// 玩家有可能在游戏开始阶段加入，也有可能在热身阶段加入
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);

	void PollInit();

	/**
	 * Default actions
	 */
	void RemoveDefaultActions();
	void AddDefaultActions();

	/**
	 * Sync Server Time
	 */
	virtual float GetServerTime(); //Sync with server world clock

	/**
	 * Gamemode State
	 */
	UFUNCTION()
	void OnRep_MatchState();
	
	
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bHighPing);
	
	FString GetInfoText(const TArray<class ABlasterPlayerState*>& PlayerStates);
	FString GetTeamsInfoText(class ABlasterGameState* BlasterGameState);
	
	void SetFlagInputState(bool bIsHoldingFlag);

protected:
	/**
	 * Get Sync Server Time and Transform the Time to SetHUDMatchCountdown()
	 */
	void SetHUDTime();

	/**
	 * Sync time between client and server
	 */
	// Reports the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TImeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// Tick
	void CheckTimeSync(float DeltaSeconds);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();
	
	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);
	
	void HighPingWarning();
	void StopHighPingWarning();
	
	void CheckPing(float DeltaTime);

private:

	void HandleMatchHasStarted();
	void HandleCooldown();
	
	class ABlasterHUD* GetSafeHUD();
	
public:
	float SingleTripTime = 0.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_ShowTeamScores)
	bool bShowTeamScores = false;
	
	FHighPingDelegate HighPingDelegate;
	
protected:

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContexts;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> FlagCarryingMappingContext;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> MouseLookMappingContexts;

	/**
	 * Sync Server Time
	 */
	float ClientServerDelta = 0.f; // Difference between server and client

	UPROPERTY(EditAnywhere, Category = Time, DisplayName = "间隔同步服务器的时间")
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	
private:
	UPROPERTY(EditAnywhere, Category = "Combat|HUD")
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;
	
	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;
	
	bool bReturnToMainMenuOpen = false;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	class UInputAction* ReturnAndQuit;
	
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	/**
	 * Enhanced Input
	 */
	UPROPERTY()
	class UEnhancedInputLocalPlayerSubsystem* InputSubsystem;
	
	/**
	 * Match Time Countdown
	 */
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float LevelStartingTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	/**
	 * Game State
	 */
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	/**
	 * Overlay
	 */
	bool bInitializeHealth = false;
	bool bInitializeGrenade = false;
	bool bInitializeScore = false;
	bool bInitializeDefeats = false;
	bool bInitializeAmmo = false;
	bool bInitializeCarriedAmmo = false;
	bool bInitializeShield = false;
	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDGrenadeCooldownTime;
	float HUDMaxGrenadeCooldownTime;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDAmmo;
	int32 HUDCarriedAmmo;

	/**
	 * Update Throw Grenade Time
	 */
	bool bCheckGrenadeCooldown = false;
	float HUDGrenadeStartTime = 0.f;


	/**
	 * Ping
	 */
	float HighPingRunningTime = 0.f;
	
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;
	
	float PingAnimationRunningTime = 0.f;
	
	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 20.f;
	
	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;
};

