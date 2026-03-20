// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "HitScanWeapon.generated.h"


/**
 * 直线跟踪武器，不用子弹来判定伤害
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

protected:
	virtual void Fire(const FVector& HitTarget) override;

	FVector TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget);
	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);

protected:
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Particles")
	class UParticleSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Particles")
	class UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Particles")
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Sound")
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Sound")
	USoundCue* HitSound;

	/**
	 * Trace end with scatter
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	float DistanceToSphere = 800.f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	bool bUseScatter = false;
	
};
