// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "BlasterComponent/CombatComponent.h"
#include "BlasterComponent/BuffComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Character/BlasterPlayerController.h"
#include "Gamemode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Character/BlasterPlayerState.h"
#include "Components/BoxComponent.h"
#include "BlasterComponent/LagCompensationComponent.h"

#define INDEX_DISSOLVE 0
#define INITIALIZE_DISSOLVE -0.55f

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	/**
	 * Controller Config
	 * Default for Initial
	 */
	bUseControllerRotationYaw = false;
	
	// Orient...To...: 使...朝向...
	GetCharacterMovement()->bOrientRotationToMovement = true;

	/**
	 * Move Config
	 */
	JumpMaxHoldTime = 0.25f;
	GetCharacterMovement()->MaxWalkSpeed = MaxRunSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = MaxCrouchSpeed;
	GetCharacterMovement()->JumpZVelocity = MaxJumpZVelocity;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 850.f, 0.f);
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	/**
	 * Component Config
	 */
	// GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_LagCompensation, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_LagCompensation, ECR_Ignore);
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = false; // 平移缓动
	CameraBoom->bEnableCameraRotationLag = false; // 转向缓动

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance = 10000.f;
	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	FollowCamera->PostProcessSettings.DepthOfFieldFstop = 32.f;
	
	
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);
	OverheadWidget->SetDrawAtDesiredSize(true);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->SetIsReplicated(true); // 组件不会自动复制

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff"));
	Buff->SetIsReplicated(true);
	
	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("Lag Compensation"));
	
	TurningInPlace = ETurningInPlace::TIP_NotTurning;
	
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline Component"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	/**
	 * Hit boxes for server-side rewind
	 */
	
	Head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	Head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), Head);
	
	Pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	Pelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("pelvis"), Pelvis);
	
	Spine = CreateDefaultSubobject<UBoxComponent>(TEXT("spine"));
	Spine->SetupAttachment(GetMesh(), FName("spine"));
	HitCollisionBoxes.Add(FName("spine"), Spine);
	
	UpperArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("upper_arm_L"));
	UpperArm_L->SetupAttachment(GetMesh(), FName("upper_arm_L"));
	HitCollisionBoxes.Add(FName("upper_arm_L"), UpperArm_L);
	
	UpperArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("upper_arm_R"));
	UpperArm_R->SetupAttachment(GetMesh(), FName("upper_arm_R"));
	HitCollisionBoxes.Add(FName("upper_arm_R"), UpperArm_R);
	
	LowerArm_L = CreateDefaultSubobject<UBoxComponent>(TEXT("lower_arm_L"));
	LowerArm_L->SetupAttachment(GetMesh(), FName("lower_arm_L"));
	HitCollisionBoxes.Add(FName("lower_arm_L"), LowerArm_L);
	
	LowerArm_R = CreateDefaultSubobject<UBoxComponent>(TEXT("lower_arm_R"));
	LowerArm_R->SetupAttachment(GetMesh(), FName("lower_arm_R"));
	HitCollisionBoxes.Add(FName("lower_arm_R"), LowerArm_R);
	
	Hand_L = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_L"));
	Hand_L->SetupAttachment(GetMesh(), FName("hand_L"));
	HitCollisionBoxes.Add(FName("hand_L"), Hand_L);
	
	Hand_R = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_R"));
	Hand_R->SetupAttachment(GetMesh(), FName("hand_R"));
	HitCollisionBoxes.Add(FName("hand_R"), Hand_R);
	
	UpperLeg_L = CreateDefaultSubobject<UBoxComponent>(TEXT("upper_leg_L"));
	UpperLeg_L->SetupAttachment(GetMesh(), FName("upper_leg_L"));
	HitCollisionBoxes.Add(FName("upper_leg_L"), UpperLeg_L);
	
	UpperLeg_R = CreateDefaultSubobject<UBoxComponent>(TEXT("upper_leg_R"));
	UpperLeg_R->SetupAttachment(GetMesh(), FName("upper_leg_R"));
	HitCollisionBoxes.Add(FName("upper_leg_R"), UpperLeg_R);
	
	LowerLeg_L = CreateDefaultSubobject<UBoxComponent>(TEXT("lower_leg_L"));
	LowerLeg_L->SetupAttachment(GetMesh(), FName("lower_leg_L"));
	HitCollisionBoxes.Add(FName("lower_leg_L"), LowerLeg_L);
	
	LowerLeg_R = CreateDefaultSubobject<UBoxComponent>(TEXT("lower_leg_R"));
	LowerLeg_R->SetupAttachment(GetMesh(), FName("lower_leg_R"));
	HitCollisionBoxes.Add(FName("lower_leg_R"), LowerLeg_R);
	
	Feet_L = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_L"));
	Feet_L->SetupAttachment(GetMesh(), FName("foot_L"));
	HitCollisionBoxes.Add(FName("foot_L"), Feet_L);
	
	Feet_R = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_R"));
	Feet_R->SetupAttachment(GetMesh(), FName("foot_R"));
	HitCollisionBoxes.Add(FName("foot_R"), Feet_R);
	
	for (auto& Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box.Value->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_LagCompensation, ECR_Block);
		}
	}
}


void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Initial for CombatComponent
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);

		Buff->SetInitialJumpZVelocity(MaxJumpZVelocity);
	}
	
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	
	GetMesh()->SetCustomPrimitiveDataFloat(INDEX_DISSOLVE, INITIALIZE_DISSOLVE);

	// 即使在 Controller 中也调用了更新血条函数，但是在 OnPossess 阶段不是所有的 HUD 变量都有效
	// 因此在角色的 BeginPlay 中还要再调用一次
	UpdateHUDHealth();
	UpdateHUDShield();
	
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}

	ShowAttachedGrenade(false);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	
	HideCameraIfCharacterClose();
	
	// 实时更新 HUD，但是没必要
	// PollInit();

	/**
	 * TODO:
	 * 当装备了狙击枪并开镜时，移动摄像机到人物中心，且进行插值移动和插值改变FOV
	 */
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// OverlappingWeapon --(Replicate)--> This Character
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}


void ABlasterCharacter::Jump()
{
	Super::Jump();
}

void ABlasterCharacter::StopJumping()
{
	Super::StopJumping();
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ABlasterCharacter::Pickup(const FInputActionValue& Value)
{
	DoPickup();
}

void ABlasterCharacter::Drop(const FInputActionValue& Value)
{
	DoDrop();
}

void ABlasterCharacter::CrouchPressed()
{
	Crouch();
}

void ABlasterCharacter::CrouchReleased()
{
	UnCrouch();
}

void ABlasterCharacter::AimPressed()
{
	if (Combat)
	{
		Combat->DoAiming(true);
	}
}

void ABlasterCharacter::AimReleased()
{
	if (Combat)
	{
		Combat->DoAiming(false);
	}
}

void ABlasterCharacter::WalkPressed()
{
	if (Combat)
	{
		Combat->DoWalking(true);
	}
}

void ABlasterCharacter::WalkReleased()
{
	if (Combat)
	{
		Combat->DoWalking(false);
	}
}

void ABlasterCharacter::FirePressed()
{
	if (Combat)
	{
		Combat->DoFiring(true);
	}
}

void ABlasterCharacter::FireReleased()
{
	if (Combat)
	{
		Combat->DoFiring(false);
	}
}

void ABlasterCharacter::Reload()
{
	if (Combat)
	{
		Combat->DoReloading();
	}
}

void ABlasterCharacter::GrenadePressed()
{
	if (Combat)
	{
		Combat->DoThrowGrenade();
	}
}


void ABlasterCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// Controller Forward
		const FRotator ControllerRotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0.f, ControllerRotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ABlasterCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

// Pickup Weapon
void ABlasterCharacter::DoPickup()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		if (Combat->CombatState == ECombatState::ECS_Unoccupied)
		{
			ServerSetEquipped();
		}
		
		bool bSwap = Combat->ShouldSwapWeapons() 
			&& !HasAuthority() 
			&& IsLocallyControlled()
			&& Combat->CombatState == ECombatState::ECS_Unoccupied
			&& OverlappingWeapon == nullptr; // 捡枪时不要播放换枪蒙太奇，否则会触发换枪通知函数
		
		if (bSwap)
		{
			Combat->SwapWeapon();
			// if (UBlasterAnimInstance* AnimInstance = GetAnimInstance())
			// {
			// 	AnimInstance->PlayEquipMontage();
			// }
			// Combat->CombatState = ECombatState::ECS_SwapWeapon;
			//
			// Combat->SetLocallySwapWeapon(false);
		}
	}
}

// Server call this function
void ABlasterCharacter::ServerSetEquipped_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Overlapping Weapon"));
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapon();
		}
	}

	if (Combat && Combat->EquippedWeapon)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;
	}
}


void ABlasterCharacter::DoDrop()
{
	
}

void ABlasterCharacter::Elim()
{
	
	DropOrDestroyWeapons();
	
	MulticastElim();
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ThisClass::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// OnSphereEndOverlap
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon = Weapon;
	
	if (OverlappingWeapon)
	{
		if (IsLocallyControlled()) // for server player
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

// Only for Client, when weapon replicated
// The old parameter "OverlappingWeapon" will be transferred in
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();

	// when hit play montage
	if (Health < LastHealth)
	{
		if (UBlasterAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();

	if (Shield < LastShield)
	{
		if (UBlasterAnimInstance* AnimInstance = GetAnimInstance())
		{
			AnimInstance->PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	UpdateHUDScore();
	UpdateHUDDefeats();
}

// Only moved, the function would be called
void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	// 由于 Simulate 不是每帧都更新动画，因此不能获取当前 frame 减去上一次地值
	SimProxiesTurn();
	
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType,
	class AController* InstigatorController, AActor* DamageCauser)
{

	if (bElimmed) return; // 防止多次进行淘汰而进行多次积分
	
	float DamageToHealth = Damage;
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - DamageToHealth, 0.f, MaxShield);
			DamageToHealth = 0.f; 
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
	
	UpdateHUDHealth();
	UpdateHUDShield();
	
	if (UBlasterAnimInstance* AnimInstance = GetAnimInstance())
	{
		AnimInstance->PlayHitReactMontage();
	}

	if (Health == 0.f)
	{
		if (ABlasterGameMode* GameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			GameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDAmmo(0);
	}

	bElimmed = true;
	
	if (UBlasterAnimInstance* AnimInstance = GetAnimInstance())
	{
		AnimInstance->PlayElimMontage();
	}

	// Start Dissolve
	StartDissolve();
	
	// Disable Movement
	SetDisableCharacterGameplay(true);
	
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->RemoveDefaultActions();
		bUseControllerRotationYaw = false;
	}

	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn Elim Bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + ElimBotOffsetZ);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
	const bool bHideSniperScope = IsLocallyControlled() && Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	if (ABlasterGameMode* GameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>())
	{
		GameMode->RequestSpawn(this, GetController());
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	GetMesh()->SetCustomPrimitiveDataFloat(INDEX_DISSOLVE, DissolveValue);
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);

	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::AddOverlappingWeapon(AWeapon* Weapon)
{
	if (Weapon)
	{
		OverlappingWeaponArray.AddUnique(Weapon);
	}
	SetOverlappingWeapon(Weapon);
	
}

void ABlasterCharacter::RemoveOverlappingWeapon(AWeapon* Weapon)
{
	if (Weapon)
	{
		OverlappingWeaponArray.Remove(Weapon);

		if (OverlappingWeapon == Weapon)
		{
			SetOverlappingWeapon(GetBestOverlappingWeapon());
		}
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !IsElimmed() && DefaultWeapon)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeapon);
		StartingWeapon->SetWeaponShouldDestroy(true);
		if (Combat)
		{
			UE_LOG(LogTemp, Warning, TEXT("Set Default Weapon: %d"), StartingWeapon != nullptr);
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon)
	{
		if (Weapon->ShouldDestroyWeapon())
		{
			Weapon->Destroy();
		}
		else
		{
			Weapon->Dropped();
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

AWeapon* ABlasterCharacter::GetBestOverlappingWeapon()
{
	if (OverlappingWeaponArray.Num() > 0)
	{
		return OverlappingWeaponArray.Last();
	}
	return nullptr;
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::IsAiming()
{
	return Combat && Combat->IsAiming();
}

bool ABlasterCharacter::IsWalking()
{
	return Combat && Combat->IsWalking();
}

// 解压缩 Pitch
void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch; // Auto Sync
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// angle will be compressed in server (-90 -> 270, 1 -> 359)
		// map pitch from [270, 360) to (-90, 0)
		// NOTE: Actor's rotation is compressed too, but preserve the original range

		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;
	return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon)
		return;

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);

		AO_Yaw = DeltaAimRotation.Yaw; // Offset Yaw
		
		if (TurningInPlace == ETurningInPlace::TIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}
	
	if (Speed > 0.f || bIsInAir) // running or jumping
	{
		bRotateRootBone = false;
		// Record the rotation before stopping
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		TurningInPlace = ETurningInPlace::TIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::TIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::TIP_Left;
	}

	// 超过旋转阈值使用插值慢慢旋转，旋转到死区重置标签
	if (TurningInPlace != ETurningInPlace::TIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::TIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::SetHealth(const float& HealthAmount)
{

	Health = FMath::Clamp(HealthAmount + Health, 0.f, MaxHealth);
}

void ABlasterCharacter::SetShield(const float& ReplenishShieldAmount)
{
	Shield = FMath::Clamp(ReplenishShieldAmount + Shield, 0.f, MaxShield);
}

bool ABlasterCharacter::IsLocallyReloading() const
{
	if (Combat)
	{
		return Combat->IsLocallyReloading();
	}
	return false;
}

bool ABlasterCharacter::IsLocallySwapWeapon() const
{
	if (Combat)
	{
		return Combat->IsLocallySwapWeapon();
	}
	return false;
}

UBlasterAnimInstance* ABlasterCharacter::GetAnimInstance() const
{
	if (UBlasterAnimInstance* AnimInstance = Cast<UBlasterAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		return AnimInstance;
	}
	return nullptr;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (!Combat)
		return nullptr;

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat)
		return FVector();

	return Combat->GetHitTarget();
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat)
		return ECombatState::ECS_MAX;

	return Combat->GetCombatState();
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	bool bIsSniperAiming = Combat && Combat->IsAiming() && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bIsSniperAiming)
	{
		SetCharacterVisibility(false);
		return;
	}

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		SetCharacterVisibility(false);
	}
	else
	{
		SetCharacterVisibility(true);
	}
}

void ABlasterCharacter::SetCharacterVisibility(bool bVisibility)
{
	if (!IsLocallyControlled()) return;
	
	GetMesh()->SetVisibility(bVisibility);
	
	ShowAttachedGrenade(bVisibility);
	
	if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
	{
		Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = !bVisibility;
	}
}

void ABlasterCharacter::ShowAttachedGrenade(bool bShowGrenade)
{
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(bShowGrenade);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (!Combat || !Combat->EquippedWeapon) return;
	
	// 对于模拟代理不需要进行旋转根骨
	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::TIP_NotTurning;
		return;
	}
	
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::TIP_Right;
		}
		else
		{
			TurningInPlace = ETurningInPlace::TIP_Left;
		}
	}	
	else
	{
		TurningInPlace = ETurningInPlace::TIP_NotTurning;
	}
}

// Tick
void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::TIP_NotTurning;
		return;
	}
	
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled()) // Autonomous Authority
	{
		AimOffset(DeltaTime);
	}
	else // Simulate / Server ( Not LocallyControlled )
	{
		// 由于 Simulate 的更新频率不是 Tick， 因此转动过程中腿部会鬼畜地抖动
		// 所以在 Simulate 不用扭腰直接转动更为自然
		
		// OnRep_ReplicatedMovement: 重载这个 Rep，并用于更新 AO_Yaw

		// 每次更新移动时都会调用这个 Rep，但是防止过长时间不更新状态，这里设定超时阈值
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			// UE_LOG(LogTemp, Warning, TEXT("simulate: %d"), TurningInPlace);
		}
	}
}


void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDAmmo(Combat->EquippedWeapon->GetCurrentAmmo());
	}
}

void ABlasterCharacter::UpdateHUDScore()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	
	if (BlasterPlayerController && BlasterPlayerState)
	{
		BlasterPlayerController->SetHUDScore(BlasterPlayerState->GetScore());
	}
}

void ABlasterCharacter::UpdateHUDDefeats()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(GetController()) : BlasterPlayerController;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
	if (BlasterPlayerController && BlasterPlayerState)
	{
		BlasterPlayerController->SetHUDDefeats(BlasterPlayerState->GetDefeats());
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}	
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);

		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		
		Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);

		Input->BindAction(PickupAction, ETriggerEvent::Triggered, this, &ThisClass::Pickup);

		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchPressed);
		Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::CrouchReleased);

		Input->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::AimPressed);
		Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::AimReleased);
		
		Input->BindAction(WalkAction, ETriggerEvent::Started, this, &ThisClass::WalkPressed);
		Input->BindAction(WalkAction, ETriggerEvent::Completed, this, &ThisClass::WalkReleased);
		
		Input->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::FirePressed);
		Input->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireReleased);
		
		Input->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::Reload);

		Input->BindAction(ThrowGrenade, ETriggerEvent::Triggered, this, &ThisClass::GrenadePressed);
		/**
		* TODO:
		* 1. 长按手雷键显示手雷抛物线
		* 2. 腾空之后再按一次空格可以冲刺，并重置腾空 CD
		*/
	}
	else
	{
		UE_LOG(LogInput, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}
}

