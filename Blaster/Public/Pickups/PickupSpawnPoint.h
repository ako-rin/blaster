// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;
class UPickupData;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupSpawnPoint();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * 处理失活事件并判断是否销毁该 SpawnPoint
	 * @param DeactivatedActor 失活的 Pickup
	 */
	UFUNCTION()
	void HandlePickupDeactivated(AActor* DeactivatedActor);

protected:
	APickup* GetAvailablePickup();

	/**
	 * 创建对象池，并将对象池中的所有对象进行隐藏（失活）
	 */
	void InitializePool();

private:
	/**
	 * 从对象池中随机抽出一个 Pickup ，并赋值及设定位置
	 */
	void ActivatePickup();

protected:
	UPROPERTY(EditAnywhere, Category = "Spawn|Object Pool")
	TSubclassOf<APickup> PickupClass;

	UPROPERTY(EditAnywhere, Category = "Spawn|Object Pool")
	int32 PoolSize = 2;

	UPROPERTY()
	TArray<APickup*> PickupPool;

	UPROPERTY(EditAnywhere, Category = "Spawn|Pickup Data")
	TArray<UPickupData*> PickupDataArray;

private:
	FTimerHandle SpawnTimer;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	float SpawnTimeMin;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	float SpawnTimeMax;

	UPROPERTY(EditAnywhere, Category = "Spawn", DisplayName = "No looger spawn the Pickup")
	bool bDestroyable = false;

	
};
