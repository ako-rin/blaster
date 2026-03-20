// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickups/Pickup.h"
#include "SpeedupPickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ASpeedupPickup : public APickup
{
	GENERATED_BODY()

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UPROPERTY(EditAnywhere, Category = "Pickup|Buff")
	float BaseSpeedBuff = 1000.f;
	
	UPROPERTY(EditAnywhere, Category = "Pickup|Buff")
	float CrouchSpeedBuff = 600.f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Buff")
	float SpeedBuffTime = 10.f;
	
};
