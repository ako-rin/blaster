// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/AmmoPickup.h"
#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		if (BlasterCharacter->GetCombat())
		{
			BlasterCharacter->GetCombat()->PickupAmmo(WeaponType, AmmoAmount);
		}
	}

	Destroy();
}
