// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	friend class ABlasterCharacter;

protected:
	virtual void BeginPlay() override;

public:
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ReplenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);

private:
	/**
	 * Heal Ramp Up
	 */
	void HealRampUp(float DeltaTime);

	/**
	 * Shield Buff
	 */
	void ReplenishRampUp(float DeltaTime);
	
	/**
	 * Speed buff
	 */
	void ResetSpeed();
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/**
	 * Jump Buff
	 */
	void ResetJump();
	void SetInitialJumpZVelocity(float Velocity);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float BaseJumpZVelocity);

	

private:	
	UPROPERTY()
	ABlasterCharacter* Character;

	/**
	 * Heal buff
	 */
	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/**
	 * Shield buff
	 */
	bool bReplenishingShield = false;
	float ShieldReplenishRate = 0.f;
	float AmountToReplenishShield = 0.f;

	/**
	 * Speed buff
	 */
	FTimerHandle SpeedBuffTimer;
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	/**
	* Jump Buff
	*/
	FTimerHandle JumpBuffTimer;
	float InitialJumpZVelocity;
		
};
