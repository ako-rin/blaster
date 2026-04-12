// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	UE_DEPRECATED(all, "Use FireShotgun() Instead")
	virtual void Fire(const FVector& HitTarget) override;
	
	virtual void FireShotgun(const TArray<FVector_NetQuantize>& HitTargets);
	void ShotgunTraceWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets);

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	uint32 NumberOfPellets = 10; // Line Traces
	
};
