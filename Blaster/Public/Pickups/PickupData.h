// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapon/WeaponTypes.h"
#include "PickupData.generated.h"

UENUM()
enum class EPickupType : uint8
{
	EPT_Health UMETA(DisplayName = "Health"),
	EPT_Shield UMETA(DisplayName = "Shield"),
	EPT_Speed UMETA(DisplayName = "Speed"),
	EPT_Jump UMETA(DisplayName = "Jump"),
	EPT_Ammo UMETA(DisplayName = "Ammo"),
	
	EPT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class BLASTER_API UPickupData : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Common
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	EPickupType PickupType;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	float BaseTurnRate = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	class UNiagaraSystem* PickupEffectVisual;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	class UNiagaraSystem* PickupEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	class UStaticMesh* PickupMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	USoundCue* PickupSound;

	/**
	 * Buff
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Health", meta = (EditCondition = "PickupType == EPickupType::EPT_Health", EditConditionHides))
	float HealAmount = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Health", meta = (EditCondition = "PickupType == EPickupType::EPT_Health", EditConditionHides))
	float HealingTime = 5.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Jump", meta = (EditCondition = "PickupType == EPickupType::EPT_Jump", EditConditionHides))
	float JumpZVelocityBuff = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Jump", meta = (EditCondition = "PickupType == EPickupType::EPT_Jump", EditConditionHides))
	float JumpBuffTime = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Speed", meta = (EditCondition = "PickupType == EPickupType::EPT_Speed", EditConditionHides))
	float BaseSpeedBuff = 1000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Speed", meta = (EditCondition = "PickupType == EPickupType::EPT_Speed", EditConditionHides))
	float CrouchSpeedBuff = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Speed", meta = (EditCondition = "PickupType == EPickupType::EPT_Speed", EditConditionHides))
	float SpeedBuffTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Shield", meta = (EditCondition = "PickupType == EPickupType::EPT_Shield", EditConditionHides))
	float ShieldReplenishAmount = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Buff Properties|Shield", meta = (EditCondition = "PickupType == EPickupType::EPT_Shield", EditConditionHides))
	float ShieldReplenishTime = 10.f;

	/**
	 * Ammo
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ammo Properties", meta = (EditCondition = "PickupType == EPickupType::EPT_Ammo", EditConditionHides))
	EWeaponType WeaponType;

	UPROPERTY(EditDefaultsOnly, Category = "Ammo Properties", meta = (EditCondition = "PickupType == EPickupType::EPT_Ammo", EditConditionHides))
	int32 AmmoAmount;
	
};
