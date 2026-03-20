// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:

	AProjectileRocket();
protected:
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult) override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

private:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class URocketMovementComponent> RocketMovementComponent;


	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Sound")
	TObjectPtr<USoundCue> ProjectileLoop;

	UPROPERTY(VisibleAnywhere, Category = "Projectile Properties|Sound")
	TObjectPtr<UAudioComponent> ProjectileAudioLoopComponent;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Sound")
	TObjectPtr<USoundAttenuation> LoopingSoundAttenuation;
	
};
