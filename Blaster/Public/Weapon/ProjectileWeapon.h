// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:
	/**
	 * 
	 * @param HitTarget From Muzzle flash socket to hit location from TraceUnderCrosshairs
	 */
	virtual void Fire(const FVector& HitTarget) override;

private:

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Projectile")
	TSubclassOf<class AProjectile> ProjectileClass;
};
