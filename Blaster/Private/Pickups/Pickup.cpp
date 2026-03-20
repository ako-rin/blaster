// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/Pickup.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "BlasterComponent/BuffComponent.h"
#include "Net/UnrealNetwork.h"
#include "Pickups/PickupData.h"
#include "Character/BlasterCharacter.h"
#include "BlasterComponent/CombatComponent.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Overlap Sphere"));
	OverlapSphere->SetupAttachment(GetRootComponent());
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlapSphere->SetSphereRadius(50.f);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pickup Mesh"));
	PickupMesh->SetupAttachment(OverlapSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	PickupMesh->MarkRenderStateDirty(); // Force flash Render
	EnableCustomDepth(true);

	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Pickup Effect Component"));
	PickupEffectComponent->SetupAttachment(GetRootComponent());
}

void APickup::BeginPlay()
{
	Super::BeginPlay();

	// only authority
	if (HasAuthority())
	{
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		OverlapSphere->OnComponentEndOverlap.AddDynamic(this,&ThisClass::OnSphereEndOverlap);
	}
	
}

void APickup::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 只用 DOREPLIFETIME 时，当 CurrentPickupData 赋值为一样的值时，为了优化而没有通知客户端
	// 则可能会产生一些显示问题。。
	// 因此需要 REPNOTIFY_Always 强制进行复制
	DOREPLIFETIME_CONDITION_NOTIFY(APickup, CurrentPickupData, COND_None, REPNOTIFY_Always);
	// DOREPLIFETIME(APickup, CurrentPickupData);
	
}

void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PickupMesh)
	{
		PickupMesh->AddWorldRotation(
			FRotator(0.f, BaseTurnRate * DeltaTime, 0.f)
		);
	}

}

void APickup::Destroyed()
{
	Super::Destroyed();
}

// authority
void APickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	bool bPickedUp = false;
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		switch (CurrentPickupData->PickupType)
		{
		case EPickupType::EPT_Health:
			if (BlasterCharacter->GetBuff())
			{
				BlasterCharacter->GetBuff()->Heal(CurrentPickupData->HealAmount, CurrentPickupData->HealingTime);
			}
			bPickedUp = true;
			break;
		case EPickupType::EPT_Jump:
			if (BlasterCharacter->GetBuff())
			{
				BlasterCharacter->GetBuff()->BuffJump(CurrentPickupData->JumpZVelocityBuff, CurrentPickupData->JumpBuffTime);
			}
			bPickedUp = true;
			break;
		case EPickupType::EPT_Shield:
			if (BlasterCharacter->GetBuff())
			{
				BlasterCharacter->GetBuff()->ReplenishShield(CurrentPickupData->ShieldReplenishAmount, CurrentPickupData->ShieldReplenishTime);
			}
			bPickedUp = true;
			break;
		case EPickupType::EPT_Speed:
			if (BlasterCharacter->GetBuff())
			{
				BlasterCharacter->GetBuff()->BuffSpeed(CurrentPickupData->BaseSpeedBuff, CurrentPickupData->CrouchSpeedBuff, CurrentPickupData->SpeedBuffTime);
			}
			bPickedUp = true;
			break;
		case EPickupType::EPT_Ammo:
			if (BlasterCharacter->GetCombat())
			{
				BlasterCharacter->GetCombat()->PickupAmmo(CurrentPickupData->WeaponType, CurrentPickupData->AmmoAmount);
			}
			bPickedUp = true;
			break;
		case EPickupType::EPT_MAX:
			break;
		}
	}

	if (bPickedUp)
	{
		MulticastPickupTriggered();

		DeactivatePickup();

		// 观察者模式
		OnPickupDeactivated.Broadcast(this); // 防止双向引用，同时避免强耦合
	}

}

void APickup::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void APickup::InitializePickup(class UPickupData* NewData)
{
	if (!HasAuthority() || !NewData) return;

	CurrentPickupData = NewData;
	ApplyPickupData();

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (OverlapSphere) OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (PickupEffectComponent)
	{
		PickupEffectComponent->Activate(true);
	}

	if (PickupMesh)
	{
		PickupMesh->SetVisibility(true);
	}

	MulticastPickupRespawned();
}

void APickup::DeactivatePickup()
{
	if (!HasAuthority()) return;

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	if (OverlapSphere) OverlapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (PickupEffectComponent)
	{
		PickupEffectComponent->DeactivateImmediate();
	}

	if (PickupMesh)
	{
		PickupMesh->SetVisibility(false);
	}
}

void APickup::ApplyPickupData()
{
	if (!CurrentPickupData) return;

	if (PickupMesh && CurrentPickupData->PickupMesh)
	{
		PickupMesh->SetStaticMesh(CurrentPickupData->PickupMesh);
		PickupMesh->SetVisibility(true);
	}

	if (PickupEffectComponent && CurrentPickupData->PickupEffectVisual)
	{
		PickupEffectComponent->SetAsset(CurrentPickupData->PickupEffectVisual);
		PickupEffectComponent->Deactivate(); // 重启
		PickupEffectComponent->Activate(true);
	}

	SetActorHiddenInGame(false);
}

void APickup::OnRep_PickupData()
{
	ApplyPickupData();
}

void APickup::MulticastPickupRespawned_Implementation()
{
	ApplyPickupData();
}

void APickup::MulticastPickupTriggered_Implementation()
{
	if (CurrentPickupData->PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			CurrentPickupData->PickupSound,
			GetActorLocation()
		);
	}

	if (CurrentPickupData->PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			CurrentPickupData->PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	if (PickupEffectComponent)
	{
		PickupEffectComponent->DeactivateImmediate();
	}

	if (PickupMesh)
	{
		PickupMesh->SetVisibility(false);
	}
}

void APickup::EnableCustomDepth(bool bEnable)
{
	if (PickupMesh)
	{
		PickupMesh->SetRenderCustomDepth(bEnable);
	}
}
