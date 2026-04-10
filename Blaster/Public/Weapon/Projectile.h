// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USoundCue;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult);

public:
	FORCEINLINE TObjectPtr<class UNiagaraSystem> GetProjectileImpact() const {return ProjectileImpact;}
	FORCEINLINE TObjectPtr<USoundCue> GetImpactSound() const {return ImpactSound;}
	FORCEINLINE class UBoxComponent* GetCollisionBox() const {return CollisionBox;}

protected:
	void SpawnTrailSystem();
	void StartDestroyTimer();
	void DeactivateTrailSystem();
	void ExplodeDamage();

private:
	void DestroyTimerFinished();
	
protected:
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Damage")
	float Damage = 20.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh = nullptr;
	
	/**
	 * Range of Damage
	 */
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Damage")
	float MinimumDamage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Damage")
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Damage")
	float DamageOuterRadius = 500.f;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Damage")
	float FalloffExponent = 1.f; 

private:

	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|FX")
	TObjectPtr<UNiagaraSystem> TrailSystem = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Projectile Properties|FX")
	TObjectPtr<class UNiagaraComponent> TrailSystemComponent = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|FX")
	TObjectPtr<UNiagaraSystem> ProjectileTracer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties|FX")
	TObjectPtr<UNiagaraSystem> ProjectileImpact = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|FX")
	TObjectPtr<UParticleSystem> ProjectileImpactParticle = nullptr;

	UPROPERTY(EditAnywhere, Category = "Projectile Properties|Sound")
	TObjectPtr<class USoundCue> ImpactSound;
	
	/**
	 * Trail Destroy Timer
	 */
	FTimerHandle DestroyTrailTimer;
	UPROPERTY(EditAnywhere, Category = "Projectile Properties|FX")
	float DestroyTime = 3.f;

};
