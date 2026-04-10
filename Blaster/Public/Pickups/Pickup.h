// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickupDeactivatedSignature, AActor*, DeactivatedActor);

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	void InitializePickup(class UPickupData* NewData);
	void DeactivatePickup();

protected:
	void ApplyPickupData();

	UFUNCTION()
	void OnRep_PickupData();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickupRespawned();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickupTriggered();
	
private:
	
	void EnableCustomDepth(bool bEnable);

public:
	UPROPERTY(BlueprintAssignable)
	FOnPickupDeactivatedSignature OnPickupDeactivated;
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_PickupData)
	UPickupData* CurrentPickupData;
	
	UPROPERTY(EditAnywhere, Category = "Pickup")
	float BaseTurnRate = 45.f;


protected:
	UPROPERTY(EditAnywhere, Category = "Pickup")
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	class UNiagaraSystem* PickupEffect;
	
	UPROPERTY(EditAnywhere, Category = "Pickup")
	class USphereComponent* OverlapSphere;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	class USoundCue* PickupSound;

};
