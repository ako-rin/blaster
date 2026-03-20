// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpawnPoint.h"
#include "Pickups/Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitializePool();

		ActivatePickup();
	}
	
}

void APickupSpawnPoint::HandlePickupDeactivated(AActor* DeactivatedActor)
{
	if (bDestroyable)
	{
		for (APickup* Pickup : PickupPool)
		{
			if (Pickup)
			{
				Pickup->Destroy();
			}
		}
		PickupPool.Empty();
		Destroy();
		return;
	}
	float SpawnTime = FMath::RandRange(SpawnTimeMin, SpawnTimeMax);
	GetWorldTimerManager().SetTimer(
		SpawnTimer,
		this,
		&ThisClass::ActivatePickup,
		SpawnTime
	);
}

APickup* APickupSpawnPoint::GetAvailablePickup()
{
	for (APickup* Pickup : PickupPool)
	{
		if (Pickup && Pickup->IsHidden())
		{
			return Pickup;
		}
	}
	return nullptr;
}

void APickupSpawnPoint::InitializePool()
{
	if (!PickupClass) return;

	for (int32 i = 0; i < PoolSize; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APickup* NewPickup = GetWorld()->SpawnActor<APickup>(
			PickupClass,
			GetActorTransform(),
			SpawnParams
		);

		if (NewPickup)
		{
			NewPickup->OnPickupDeactivated.AddDynamic(this, &ThisClass::HandlePickupDeactivated);

			NewPickup->DeactivatePickup();

			PickupPool.Add(NewPickup);
		}
	}
}

void APickupSpawnPoint::ActivatePickup()
{
	if (PickupDataArray.Num() == 0) return;

	int32 Selection = FMath::RandRange(0, PickupDataArray.Num() - 1);
	UPickupData* SelectedData = PickupDataArray[Selection];

	APickup* AvailablePickup = GetAvailablePickup();

	if (AvailablePickup)
	{
		AvailablePickup->SetActorLocation(GetActorLocation());
		AvailablePickup->SetActorRotation(GetActorRotation());

		AvailablePickup->InitializePickup(SelectedData);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup Pool Exhausted!"));
	}
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

