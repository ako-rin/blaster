// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterAnimInstance.h"

#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterComponent/CombatState.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!BlasterCharacter)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if (!BlasterCharacter)
		return;

	FVector Velocity = BlasterCharacter->GetVelocity();
	float Speed = Velocity.Size2D();
	SpeedXY = FMath::FInterpTo(SpeedXY, Speed, DeltaSeconds, 5.f);
	SpeedZ = Velocity.Z;
	
	// GetMovementComponent -> PawnMovementComponent
	// GetCharacterMovement -> CharacterMovementComponent
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;

	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();

	bIsCrouched = BlasterCharacter->bIsCrouched;

	bIsAiming = BlasterCharacter->IsAiming();

	bIsWalking = BlasterCharacter->IsWalking();

	bIsLocal = BlasterCharacter->IsLocallyControlled();

	bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();

	TurningInPlace = BlasterCharacter->GetTurningInPlace();

	bElimmed = BlasterCharacter->IsElimmed();
	
	// Offset Yaw Strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation(); // World Axis
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); // Local Axis
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 5.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch = BlasterCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		// Get LeftHandSocket Transform
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);

		// 计算 hand_R 参考系下的相对坐标，相对于右手不动
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(
			FName("hand_R"),
			LeftHandTransform.GetLocation(),
			FRotator::ZeroRotator,
			// LeftHandTransform.GetLocation().Rotation(),
			OutPosition,
			OutRotation
		);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// 只需要本地看着正常就行了
		if (BlasterCharacter->IsLocallyControlled())
		{
			FTransform RightHandTransform = BlasterCharacter->GetMesh()->GetSocketTransform(FName("hand_R"), RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
			LookAtRotation = FRotator(LookAtRotation.Pitch, LookAtRotation.Yaw, LookAtRotation.Roll - 90.f);

			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}

		Debug_DrawMuzzleLine(BlasterCharacter->IsDrawDebugMuzzleLine());
	}

	bUseFABRIK = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bTransformRightHand = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableCharacterGameplay();
	bUseAimOffsets = BlasterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !BlasterCharacter->GetDisableCharacterGameplay();
}

void UBlasterAnimInstance::PlayFireMontage()
{
	if (!BlasterCharacter || !BlasterCharacter->GetEquippedWeapon())
	{
		return;
	}
	
	if (FireMontage)
	{
		Montage_Play(FireMontage);
		Montage_JumpToSection(bIsAiming ? FName("RifleAim") : FName("RifleHip"));
	}
}

void UBlasterAnimInstance::PlayReloadMontage()
{
	if (!BlasterCharacter || !BlasterCharacter->GetEquippedWeapon())
	{
		return;
	}
	
	if (ReloadMontage)
	{
		Montage_Play(ReloadMontage);
		FName SectionName;

		switch (EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;

			case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;

			case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;

			case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
			
			case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
			
			case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Sniper");
			break;

			
			case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("Rifle");
			break;

		case EWeaponType::EWT_MAX:
			SectionName = FName("Rifle");
			break;

		default:
			SectionName = FName("Rifle");
			break;
		}

		Montage_JumpToSection(SectionName);
	}
}

void UBlasterAnimInstance::PlayHitReactMontage()
{
	if (!BlasterCharacter || !BlasterCharacter->GetEquippedWeapon())
	{
		return;
	}
	
	if (HitReactMontage)
	{
		Montage_Play(HitReactMontage);
		Montage_JumpToSection(FName("FromFront"));
		// UE_LOG(LogTemp, Warning, TEXT("Play Hit Montage"));
		// Montage_JumpToSection(bIsAiming ? FName("RifleAim") : FName("RifleHip"));
	}
}

void UBlasterAnimInstance::PlayElimMontage()
{
	if (!BlasterCharacter) return;

	if (ElimMontage)
	{
		Montage_Play(ElimMontage);
		Montage_JumpToSection(FName("Default"));
	}
}

void UBlasterAnimInstance::PlayThrowGrenadeMontage()
{
	if (!BlasterCharacter) return;

	if (ThrowGrenadeMontage)
	{
		Montage_Play(ThrowGrenadeMontage);
		Montage_JumpToSection(FName("Default"));
	}
}

void UBlasterAnimInstance::PlayShotgunReloadEndMontage()
{
	if (!BlasterCharacter) return;

	if (ReloadMontage)
	{
		Montage_Play(ReloadMontage);
		Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UBlasterAnimInstance::AnimNotify_FootStepL()
{
	if (!BlasterCharacter) return;

	bool bIsMoving = SpeedXY > 10.f;
	bool bIsRunning = !bIsAiming && !bIsWalking && !bIsInAir && bIsMoving;
	
	if (FootStepL && bIsRunning)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FootStepL,
			BlasterCharacter->GetMesh()->GetSocketLocation(FName("foot_L"))
		);
	}
}

void UBlasterAnimInstance::AnimNotify_FootStepR()
{
	if (!BlasterCharacter) return;
	
	bool bIsMoving = SpeedXY > 10.f;
	bool bIsRunning = !bIsAiming && !bIsWalking && !bIsInAir && bIsMoving;
	
	if (FootStepR && bIsRunning)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FootStepR,
			BlasterCharacter->GetMesh()->GetSocketLocation(FName("foot_R"))
		);
	}
}

void UBlasterAnimInstance::AnimNotify_FinishReloading()
{
	if (!BlasterCharacter || !BlasterCharacter->GetCombat())
		return;

	BlasterCharacter->GetCombat()->FinishReloading();
}

void UBlasterAnimInstance::AnimNotify_Shell()
{
	if (!BlasterCharacter || !BlasterCharacter->GetCombat())
		return;

	BlasterCharacter->GetCombat()->ShotgunShellReload();
}

void UBlasterAnimInstance::AnimNotify_FinishedGrenadeThrow()
{
	if (!BlasterCharacter || !BlasterCharacter->GetCombat())
		return;

	BlasterCharacter->GetCombat()->ThrowGrenadeFinished();
}

void UBlasterAnimInstance::AnimNotify_GrenadeLaunch()
{
	if (!BlasterCharacter || !BlasterCharacter->GetCombat())
		return;

	BlasterCharacter->GetCombat()->LaunchGrenade();
}

void UBlasterAnimInstance::Debug_DrawMuzzleLine(const bool& bDrawMuzzleLine) const
{
	if (!bDrawMuzzleLine) return;
	FTransform  MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Muzzle"), RTS_World);
	FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
	DrawDebugLine(GetWorld(),MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
	DrawDebugLine(GetWorld(),MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Cyan);
	DrawDebugSphere(GetWorld(),BlasterCharacter->GetHitTarget(), 5.f, 32, FColor::Orange);
}
