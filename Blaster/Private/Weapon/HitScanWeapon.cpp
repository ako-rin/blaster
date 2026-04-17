// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/HitScanWeapon.h"

#include "BlasterComponent/LagCompensationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// InstigatorController is not valid on the proxy
	// Fire is called on the server
	
	// APawn* OwnerPawn = Cast<APawn>(GetOwner());
	// if (OwnerPawn == nullptr) return;
	// AController* InstigatorController = OwnerPawn->GetController();
	
	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("Muzzle")))
	{
		const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor()))
		{
			if (GetInstigatorController())
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || GetInstigator()->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{
					UGameplayStatics::ApplyDamage(
						BlasterCharacter,
						Damage,
						GetInstigatorController(),
						this,
						UDamageType::StaticClass()
					);
				}
				if (!HasAuthority() && bUseServerSideRewind)
				{
					BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetInstigator()) : BlasterOwnerCharacter;
					BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(GetInstigatorController()) : BlasterOwnerController;
					if (BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensation())
					{
						BlasterOwnerCharacter->GetLagCompensation()->ServerScoreRequest(
							BlasterCharacter,
							Start,
							HitTarget,
							BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime,
							this
						);
					}
				}
			}
		}
		if (FireHit.bBlockingHit)
		{
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint
				);
			}
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactPoint.Rotation()
				);
			}
		}
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	if (UWorld* World = GetWorld())
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		
		// DrawDebugSphere(GetWorld(), BeamEnd, 16.f, 12, FColor::Orange, true);
		
		if (BeamParticles)
		{
			UParticleSystemComponent * Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamParticles,
					TraceStart,
					FRotator::ZeroRotator,
					true
				);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}
