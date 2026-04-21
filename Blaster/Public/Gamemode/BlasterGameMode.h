// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown; // Match duration has been matched. Display winner and begin cooldown timer.
}

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ABlasterGameMode();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestSpawn(class ACharacter* ElimmedCharacter, class AController* ElimmedController);
	
	void PlayerLeftGame(class ABlasterPlayerState* PlayerLeaving);
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	FORCEINLINE float GetWarmupTime() const {return WarmupTime;}
	FORCEINLINE float GetMatchTime() const {return MatchTime;}
	FORCEINLINE float GetCooldownTime() const {return CooldownTime;}
	FORCEINLINE float GetLevelStartingTime() const {return LevelStartingTime;}
	FORCEINLINE float GetCountdownTime() const {return CountdownTime;}
	
public:
	bool bTeamsMatch = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
	
private:
	float CountdownTime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;
	
	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	// BeginPlay time
	UPROPERTY(EditDefaultsOnly)
	float LevelStartingTime = 0.f;
};
