// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterComponent/CombatComponent.h"
#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Character/BlasterPlayerController.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Weapon/Projectile.h"
#include "Weapon/Shotgun.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// FHitResult HitResult;
	// TraceUnderCrosshairs(HitResult);
	// DrawDebugSphere(GetWorld(),HitResult.ImpactPoint, 15,15,FColor::Red);


	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (Character->GetFollowCamera())
	{
		DefaultFOV = Character->GetFollowCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
	if (Character->HasAuthority())
	{
		InitializeCarriedAmmo();
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDGrenadeCooldown(GrenadeCooldownTime, GrenadeCooldownTime);
	}

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, SecondaryWeapon);
	DOREPLIFETIME_CONDITION(UCombatComponent, bAiming, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(UCombatComponent, bWalking, COND_SimulatedOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly); // only for client
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		// 如果已经装备了武器，就丢掉当前地武器
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach) return;
	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach) return;
	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("LeftHandSocket"));
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (!Character || !Character->GetMesh() || !ActorToAttach) return;
	const USkeletalMeshSocket* RightHandSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

int32 UCombatComponent::GetCarriedAmmoFromAmmoMap()
{
	if (EquippedWeapon && CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	return CarriedAmmo;
}

// Updating Carried Ammo and Updating Carried Ammo HUD
void UCombatComponent::UpdateCarriedAmmo()
{
	if (!EquippedWeapon) return;
	GetCarriedAmmoFromAmmoMap();
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquippedWeaponSound(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip && WeaponToEquip->GetEquippedSound())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WeaponToEquip->GetEquippedSound(),
			Character->GetActorLocation()
		);
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->IsEmptyAmmo())
	{
		DoReloading();
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->ShowAttachedGrenade(bShowGrenade);
	}
}

// Notify
void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);

	if (Character && Character->IsLocallyControlled())
	{
		// 生成的手榴弹（GrenadeProjectile）是可复制的，因此由服务端来生成可以让所有端都能看见
		// 如果是本地生成，由于 HitTarget 是本地计算的，服务端始终为 0 点，则会出现投掷方向出问题
		ServerLaunchGrenade(HitTarget); 
	}

}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		if (UWorld* World = GetWorld())
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParameters
			);
		}
	}
}

void UCombatComponent::EquipWeapon(class AWeapon* WeaponToEquip)
{
	if (!Character || !WeaponToEquip) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (EquippedWeapon && !SecondaryWeapon)
	{
		EquipSecondaryWeapon(WeaponToEquip);
	}
	else
	{
		EquipPrimaryWeapon(WeaponToEquip);
	}


	// Character Control
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::SwapWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	AWeapon* TempWeapon = EquippedWeapon;
	EquippedWeapon = SecondaryWeapon;
	SecondaryWeapon = TempWeapon;
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	PlayEquippedWeaponSound(EquippedWeapon);
	ReloadEmptyWeapon();

	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	AttachActorToBackpack(SecondaryWeapon);
	
}

bool UCombatComponent::ShouldSwapWeapons()
{
	return EquippedWeapon && SecondaryWeapon;
}

void UCombatComponent::EquipPrimaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	// Equip Weapon
	DropEquippedWeapon();

	EquippedWeapon = WeaponToEquip; // Call OnRep
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped); // Call OnRep
	
	AttachActorToRightHand(EquippedWeapon);

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetInstigator(Character);
	
	EquippedWeapon->SetHUDAmmo();

	// Update HUD and CarriedAmmo
	UpdateCarriedAmmo();

	// Play Equipped Sound
	PlayEquippedWeaponSound(EquippedWeapon);

	// Reload Weapon if Ammo is Empty
	ReloadEmptyWeapon();

}

void UCombatComponent::EquipSecondaryWeapon(AWeapon* WeaponToEquip)
{
	if (!WeaponToEquip) return;
	SecondaryWeapon = WeaponToEquip;
	SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
	
	AttachActorToBackpack(SecondaryWeapon);
	PlayEquippedWeaponSound(SecondaryWeapon);
	
	SecondaryWeapon->SetOwner(Character);
	SecondaryWeapon->SetInstigator(Character);

}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		// 由于 EquippedWeapon 和 WeaponState 都是同步量，同步过程不能保证哪个 OnRep 先调用
		// 防止出现捡起武器时，武器还有物理状态而掉落的错误状态
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		PlayEquippedWeaponSound(EquippedWeapon);

		EquippedWeapon->SetHUDAmmo();

		EquippedWeapon->EnableCustomDepth(false);
	}
}

void UCombatComponent::OnRep_SecondaryWeapon()
{
	if (SecondaryWeapon && Character)
	{
		SecondaryWeapon->SetWeaponState(EWeaponState::EWS_EquippedSecondary);
		AttachActorToRightHand(SecondaryWeapon);

		PlayEquippedWeaponSound(SecondaryWeapon);
		
	}
}

void UCombatComponent::OnRep_Aiming()
{
	if (!EquippedWeapon)
		return;
	
	UE_LOG(LogTemp, Warning, TEXT("OnRep_Aiming"));
	UpdateMoveSpeed(); // for Simulate and Autonomous
}

void UCombatComponent::OnRep_Walking()
{
	UpdateMoveSpeed(); // for Simulate and Autonomous	
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon &&
			EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
				CarriedAmmo == 0;
	if (bJumpToShotgunEnd && Character->GetAnimInstance())
	{
		Character->GetAnimInstance()->PlayShotgunReloadEndMontage();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFiring)
		{
			Fire();
		}
		break;
	case ECombatState::ECS_ThrowingGrenade:
		if (Character && !Character->IsLocallyControlled() && Character->GetAnimInstance())
		{
			ShowAttachedGrenade(true);
			Character->GetAnimInstance()->PlayThrowGrenadeMontage();
			AttachActorToLeftHand(EquippedWeapon);
		}
		break;
	case ECombatState::ECS_MAX:
		break;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	
	bAiming = bIsAiming;
	UpdateMoveSpeed();
}

void UCombatComponent::ServerSetWalking_Implementation(bool bIsWalking)
{
	bWalking = bIsWalking;
	UpdateMoveSpeed();
}

void UCombatComponent::ServerFiring_Implementation(bool bIsFiring, const FVector_NetQuantize& TraceHitResult)
{
	if (bIsFiring)
	{
		MultiCastFiring(bIsFiring, TraceHitResult);
	}
}


void UCombatComponent::LocalFire(bool bIsFiring, const FVector_NetQuantize& TraceHitTarget)
{
	if (!EquippedWeapon) return;
	
	if (Character && bIsFiring && CombatState == ECombatState::ECS_Unoccupied) // 防止换弹时还能继续开枪
	{
		if (UBlasterAnimInstance* AnimInstance = Character->GetAnimInstance())
		{
			AnimInstance->PlayFireMontage();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Can't get AnimInstance"));
		}
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ShotgunLocalFire(bool bIsFiring, const TArray<FVector_NetQuantize>& TraceHitTargets)
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun == nullptr || Character == nullptr) return;
	
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		if (UBlasterAnimInstance* AnimInstance = Character->GetAnimInstance())
		{
			AnimInstance->PlayFireMontage();
		}
		Shotgun->FireShotgun(TraceHitTargets);
		CombatState = ECombatState::ECS_Unoccupied;
	}
}

void UCombatComponent::MultiCastFiring_Implementation(bool bIsFiring, const FVector_NetQuantize& TraceHitResult)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	LocalFire(bIsFiring, TraceHitResult);
}

void UCombatComponent::ServerShotgunFiring_Implementation(bool bIsFiring,
	const TArray<FVector_NetQuantize>& TraceHitResults)
{
	MulticastShotgunFiring(bIsFiring, TraceHitResults);
}

void UCombatComponent::MulticastShotgunFiring_Implementation(bool bIsFiring,
	const TArray<FVector_NetQuantize>& TraceHitResults)
{
	if (Character && Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	ShotgunLocalFire(bIsFiring, TraceHitResults);
}


void UCombatComponent::ServerReload_Implementation()
{
	if (!Character || !EquippedWeapon) return;
	
	// Reload
	
	// 服务端不在空闲状态不给换弹
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	// 服务端当前弹夹子弹已满不给换弹
	if (EquippedWeapon->GetCurrentAmmo() >= EquippedWeapon->GetMagCapacity()) return;
	
	// 服务端备弹为0时不给换弹
	int32 CarriedAmountAmmo = 0;
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmountAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	if (CarriedAmountAmmo <= 0) return;
	
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
	
}

void UCombatComponent::ServerShotGunReload_Implementation()
{
	UpdateShotGunAmmoValues();
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	if (!bCanThrowGrenade) return;
	
	bCanThrowGrenade = false;
	StartGrenadeCooldownTimer();
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character && Character->GetAnimInstance())
	{
		ShowAttachedGrenade(true);
		Character->GetAnimInstance()->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
	}
	
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount)
{
	if (CarriedAmmoMap.Contains(WeaponType))
	{
		CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxCarriedAmmo);

		UpdateCarriedAmmo();
	}

	if (EquippedWeapon && EquippedWeapon->IsEmptyAmmo() && EquippedWeapon->GetWeaponType() == WeaponType)
	{
		ServerReload();
	}
	
}

void UCombatComponent::DoAiming(bool bIsAiming)
{
	if (!Character || !EquippedWeapon)
		return;
	
	bAiming = bIsAiming;
	UpdateMoveSpeed();
	UE_LOG(LogTemp, Warning, TEXT("Local Aiming"));
	if (Character && !Character->HasAuthority())
	{
		ServerSetAiming(bIsAiming);
	}
	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::DoWalking(bool bIsWalking)
{
	bWalking = bIsWalking;
	UpdateMoveSpeed();
	if (Character && !Character->HasAuthority())
		ServerSetWalking(bIsWalking);
}

void UCombatComponent::DoFiring(bool bIsFiring)
{
	if (!EquippedWeapon)
		return;
	
	bFiring = bIsFiring;

	if (bFiring)
	{
		Fire();
	}
}

void UCombatComponent::DoReloading()
{
	if (!EquippedWeapon)
		return;

	if (EquippedWeapon->GetMagCapacity() > EquippedWeapon->GetCurrentAmmo() && CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied)
	{
		ServerReload();
	}
}

void UCombatComponent::DoThrowGrenade()
{
	if (CombatState != ECombatState::ECS_Unoccupied || !EquippedWeapon || !bCanThrowGrenade) return;

	bCanThrowGrenade = false;
	StartGrenadeCooldownTimer();
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		LastThrowGrenadeTime = Controller->GetServerTime();
		Controller->StartGrenadeCooldown(LastThrowGrenadeTime);
	}
	
	CombatState = ECombatState::ECS_ThrowingGrenade;
	if (Character && Character->GetAnimInstance())
	{
		ShowAttachedGrenade(true);
		Character->GetAnimInstance()->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
	}
	if (Character && !Character->HasAuthority())
	{
		// show grenade and weapon attach to left hand
		ServerThrowGrenade();
	}
}

void UCombatComponent::Fire()
{
	if (EquippedWeapon->IsEmptyAmmo() && bCanFire &&
			CombatState == ECombatState::ECS_Unoccupied)
	{
		DoReloading();
	}
	
	if (!EquippedWeapon || !CanFire())
	{
		return;
	}
	
	bCanFire = false;
	
	switch (EquippedWeapon->GetFireType())
	{
	case EFireType::EFT_Projectile:
		FireProjectileWeapon();
		break;
	case EFireType::EFT_HitScan:
		FireHitScanWeapon();
		break;
	case EFireType::EFT_Shotgun:
		FireShotgun();
		break;
	case EFireType::EFT_MAX:
		break;
	}
	
	// 通过 RPC 传输变量给服务器
	// ServerFiring(bFiring, HitTarget);
	//
	// LocalFire(bFiring, HitTarget);
	
	CrosshairFireFactor += 0.80f;
	
	StartFireTimer();
}

void UCombatComponent::FireProjectileWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->IsUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
	    if (!Character->HasAuthority()) LocalFire(bFiring, HitTarget);
		ServerFiring(bFiring, HitTarget);
	}
}

void UCombatComponent::FireHitScanWeapon()
{
	if (EquippedWeapon && Character)
	{
		HitTarget = EquippedWeapon->IsUseScatter() ? EquippedWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		if (!Character->HasAuthority())	LocalFire(bFiring, HitTarget);
		ServerFiring(bFiring, HitTarget);
	}
}

void UCombatComponent::FireShotgun()
{
	AShotgun* Shotgun = Cast<AShotgun>(EquippedWeapon);
	if (Shotgun && Character)
	{
		TArray<FVector_NetQuantize> HitTargets;
		Shotgun->ShotgunTraceWithScatter(HitTarget, HitTargets);
		if (!Character->HasAuthority()) ShotgunLocalFire(bFiring, HitTargets);
		ServerShotgunFiring(bFiring, HitTargets);
	}
}

// Play Reload Montage
void UCombatComponent::HandleReload()
{
	if (UBlasterAnimInstance* AnimInstance = Cast<UBlasterAnimInstance>(Character->GetAnimInstance()))
	{
		AnimInstance->PlayReloadMontage();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (!Character || !EquippedWeapon) return;
	
	// Calculate Ammo
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	// Update Ammo HUD
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotGunAmmoValues()
{
	if (!Character || !EquippedWeapon) return;

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(1);

	bCanFire = true; // 不需要经过完全的时间才能开枪
	
	if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
	{
		// Jump to ShotgunEnd section
		if (Character->GetAnimInstance())
		{
			Character->GetAnimInstance()->PlayShotgunReloadEndMontage();
		}
	}
}

void UCombatComponent::StartGrenadeCooldownTimer()
{
	if (!Character) return;
	
	Character->GetWorldTimerManager().SetTimer(
		GrenadeTimer,
		this,
		&ThisClass::GrenadeCooldownFinished,
		GrenadeCooldownTime
	);
}

void UCombatComponent::GrenadeCooldownFinished()
{
	bCanThrowGrenade = true;
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize(0.f, 0.f);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		// 避免相机到角色之间的 Actor 被追踪
		if (Character)
		{
			float CameraDistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (CameraDistanceToCharacter + TraceStartBias);
		}
		
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility
		);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

// TICK
void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (!Character || !Character->GetController()) return;

	Controller = Controller == nullptr ?
		Cast<ABlasterPlayerController>(Character->GetController()) : Controller;

	if (Controller)
	{
		HUD = HUD == nullptr ?
			Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairsCenter;	
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;	
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;	
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;	
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;	
				HUDPackage.CrosshairsLeft = nullptr;	
				HUDPackage.CrosshairsRight = nullptr;	
				HUDPackage.CrosshairsBottom = nullptr;	
				HUDPackage.CrosshairsTop = nullptr;
			}

			// Calculate crosshair spread

			// Velocity Factor
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetMaxRunSpeed());
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			// Jump Factor
			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairFireFactor = FMath::FInterpTo(CrosshairFireFactor, 0.f, DeltaTime, 15.f);
			
			HUDPackage.CrosshairSpread = 0.5f
										+ CrosshairVelocityFactor
										+ CrosshairInAirFactor
										- CrosshairAimFactor
										+ CrosshairFireFactor;
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (!EquippedWeapon) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (!EquippedWeapon || !Character) return;

	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&ThisClass::FireTimerFinished,
		EquippedWeapon->GetFireDelay()
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (!EquippedWeapon || !Character) return;
	
	bCanFire = true;
	if (bFiring && EquippedWeapon->GetAutomaticFire())
	{
		Fire();
	}
	if (bFiring && EquippedWeapon->IsEmptyAmmo())
	{
		DoReloading();
	}
}

bool UCombatComponent::CanFire()
{
	if (!EquippedWeapon) return false;

	if (!EquippedWeapon->IsEmptyAmmo() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		return true;
	}

	return !EquippedWeapon->IsEmptyAmmo() &&
		bCanFire &&
			CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketLauncherAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSubmachineGunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
}

int32 UCombatComponent::AmountToReload()
{
	if (!EquippedWeapon) return 0;

	// 需填装的弹药数量
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetCurrentAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}

	return 0;
}

// Notify
void UCombatComponent::FinishReloading()
{
	if (!Character) return;

	if (Character->HasAuthority())
	{
		// 权威来设置状态，客户端仅被复制，不能主动更改状态	
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	// 可能由于网络延时导致客户端的状态还没转换到 Unoccupied 就导致 CanFire() 依旧返回 false
	// 就导致下面的 Fire 函数在本地可能不会触发，因此需要在 OnRep 中再进行一次 Fire()
	if (bFiring)
	{
		Fire();
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		// For the Server
		UpdateShotGunAmmoValues();
	}
	// if (Character && Character->IsLocallyControlled())
	// {
	// 	// Request for the server
	// 	ServerShotGunReload();
	// }
}

void UCombatComponent::UpdateMoveSpeed()
{
	if (!Character || !Character->GetCharacterMovement())
		return;

	Character->GetCharacterMovement()->MaxWalkSpeed = bWalking ? Character->GetMaxRunSpeed() * WalkingFactor :
														bAiming ? Character->GetMaxRunSpeed() * AimingFactor :
																	Character->GetMaxRunSpeed();
}
