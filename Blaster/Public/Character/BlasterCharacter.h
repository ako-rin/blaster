// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "BlasterType/TurningPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterComponent/CombatState.h"
#include "BlasterType/Team.h"
#include "BlasterCharacter.generated.h"

class UInputAction;
struct FInputActionValue;
class AWeapon;
class UCameraComponent;
class UCombatComponent;
class UBuffComponent;
class UBlasterAnimInstance;
class USoundCue;
class ULagCompensationComponent;

#define CUSTOM_TAN_OUTLINE 254

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void BeginPlay() override;
	
	// When Controller Attach the Character, PossessedBy will be called
	// Only on Server
	virtual void PossessedBy(AController* NewController) override;

	virtual void Jump() override;
	virtual void StopJumping() override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Pickup(const FInputActionValue& Value);
	void Drop(const FInputActionValue& Value);
	void CrouchPressed();
	void CrouchReleased();
	void AimPressed();
	void AimReleased();
	void WalkPressed();
	void WalkReleased();
	void FirePressed();
	void FireReleased();
	void Reload();
	void GrenadePressed();

	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);
	void DoPickup();
	void DoDrop();
	

public:

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UFUNCTION()
	void OnRep_Health(float LastHealth); // OnRep 复制比 RPC 更有效
	
	UFUNCTION()
	void OnRep_Shield(float LastShield); // OnRep 复制比 RPC 更有效
	
	virtual void OnRep_PlayerState() override;

	virtual void OnRep_ReplicatedMovement() override;
	
	UFUNCTION(Server, Reliable)
	void ServerLeaveGame();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	void Elim(bool bPlayerLeftGame);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainedTheLead();
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLostTheLead();
	
	void SetOverlappingWeapon(AWeapon* Weapon);
	
	bool IsWeaponEquipped();
	bool IsAiming();
	bool IsWalking();
	
	void CalculateAO_Pitch();
	float CalculateSpeed();

	void AimOffset(float DeltaTime);
	void TurnInPlace(float DeltaTime);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	void UpdateHUDScore();
	void UpdateHUDDefeats();

	// Poll for any relevant classes and initialize HUD
	void PollInit();
	
	FORCEINLINE float GetAO_Yaw() const {return AO_Yaw;}
	FORCEINLINE float GetAO_Pitch() const {return AO_Pitch;}
	FORCEINLINE ETurningInPlace GetTurningInPlace() const {return TurningInPlace;}

	FORCEINLINE float GetMaxRunSpeed() const {return MaxRunSpeed;}
	FORCEINLINE float GetMaxCrouchSpeed() const {return MaxCrouchSpeed;}

	FORCEINLINE UCameraComponent* GetFollowCamera() const {return FollowCamera;}

	FORCEINLINE UCombatComponent* GetCombat() const {return Combat;}
	FORCEINLINE UBuffComponent* GetBuff() const {return Buff;}

	FORCEINLINE bool ShouldRotateRootBone() const {return bRotateRootBone;}

	FORCEINLINE bool IsElimmed() const {return bElimmed;}

	FORCEINLINE float GetHealth() const {return Health;}
	FORCEINLINE float GetMaxHealth() const {return MaxHealth;}
	void SetHealth(const float& HealthAmount);

	FORCEINLINE float GetShield() const {return Shield;}
	FORCEINLINE float GetMaxShield() const {return MaxShield;}
	void SetShield(const float& ReplenishShieldAmount);

	FORCEINLINE void SetDisableCharacterGameplay(const bool& b) {bDisableGameplay = b;}
	FORCEINLINE bool GetDisableCharacterGameplay() const {return bDisableGameplay;}

	FORCEINLINE bool IsDrawDebugMuzzleLine() const {return bDrawMuzzleLine;}

	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const {return AttachedGrenade;}

	FORCEINLINE AWeapon* GetOverlappingWeapon() const {return OverlappingWeapon;}
	
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const {return LagCompensation;}
	
	FORCEINLINE ETeam GetTeam();
	
	FORCEINLINE bool IsHoldingTheFlag() const;
	void SetHoldingTheFlag(bool bHolding);
	
	bool IsLocallyReloading() const;
	bool IsLocallySwapWeapon() const;

	UBlasterAnimInstance* GetAnimInstance() const;
	
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
	
	ECombatState GetCombatState() const;

	/**
	 * Camera and Visibility
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);
	void SetCharacterVisibility(bool bVisibility);
	void ShowAttachedGrenade(bool bShowGrenade);

	/**
	 * Temporarily Save Overlapping Weapon
	 */
	AWeapon* GetBestOverlappingWeapon();
	void AddOverlappingWeapon(AWeapon* Weapon);
	void RemoveOverlappingWeapon(AWeapon* Weapon);
	void EmptyOverlappingWeapon();

	/**
	 *  Weapon
	 */
	void SpawnDefaultWeapon();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();
	
	void SetSpawnPoint();
	void OnPlayerStateInitialized();

	/**
	 * Team
	 */
	void UpdateTeamOutline();
	
	bool HasTeamFlag();

private:
	
	UFUNCTION(Server, Reliable)
	void ServerSetEquipped();

	/**
	 * Camera and Visibility
	 */
	// 如果摄像机十分靠墙就会被角色自己挡住，因此需要暂时隐藏
	void HideCameraIfCharacterClose();

	/**
	 * 用于模拟代理时更顺滑地旋转
	 */
	void SimProxiesTurn();
	void RotateInPlace(float DeltaTime);

	/**
	 * Health
	 */
	UFUNCTION()
	void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	void ElimTimerFinished();

	/**
	* Dissolve Effect 
	*/
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	
	void StartDissolve();
	
	
public:
		
	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;
	
	bool bLeftGame = false;
	FOnLeftGame OnLeftGame;
	
protected:
		
	/**
	 * Hit Boxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Head;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Pelvis;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Spine;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperArm_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperArm_R;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerArm_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerArm_R;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Hand_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Hand_R;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperLeg_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* UpperLeg_R;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerLeg_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* LowerLeg_R;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Feet_L;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* Feet_R;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* MoveAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* PickupAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* CrouchAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* AimAction;

	// 静步
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* WalkAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* FireAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* ReloadAction;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Input")
	UInputAction* ThrowGrenade;


	/**
	 * Movement
	 */

	UPROPERTY(EditAnywhere, Category = "Combat|Movement")
	float MaxRunSpeed = 350.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Movement")
	float MaxCrouchSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Movement")
	float MaxJumpZVelocity = 180.f;
	
	/**
	 * Player Health and Shield
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Player State")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Health, EditAnywhere, Category = "Combat|Player State")
	float Health = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Player State")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Shield, EditAnywhere, Category = "Combat|Player State")
	float Shield = 0.f;

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
	
private:
	
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;
	
	UPROPERTY(VisibleAnywhere)
	ULagCompensationComponent* LagCompensation;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	
	/**
	 * Overlapping Weapon
	 */
	
	// ReplicatedUsing support none or one parameter (Old parameter)
	// OverlappingWeapon changed, OnRep_OverlappingWeapon will be called
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TArray<AWeapon*> OverlappingWeaponArray;

	/**
	 * Rotation
	 */
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurningInPlace TurningInPlace;
	
	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float CameraThreshold = 150.f;

	/**
	 * Elim
	 */
	bool bElimmed = false; // 被淘汰
	
	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Elim")
	float ElimDelay = 3.f;

	/**
	 * Dissolve Effect    
	 */
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	// Dynamic Instance that we can change at runtime
	// UPROPERTY(VisibleAnywhere, Category = "Combat|Elim")
	// UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	//
	// // Material Instance set on the Blueprint, used with the dynamic material instance
	// UPROPERTY(EditAnywhere, Category = "Combat|Elim")
	// UMaterialInstance* DissolveMaterialInstance;

	/**
	 * Elim Bot
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Elim")
	float ElimBotOffsetZ = 100.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Elim")
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = "Combat|Elim")
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere, Category = "Combat|Elim")
	USoundCue* ElimBotSound;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Crown")
	class UNiagaraSystem* CrownSystem;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Crown")
	float CrownOffsetZ = 20.f;
	
	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;
	
	/**
	 * Gameplay
	 */
	UPROPERTY(VisibleAnywhere)
	bool bDisableGameplay = false;

	/**
	 * Debug
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|DEBUG")
	bool bDrawMuzzleLine = false;

	/**
	* Grenade
	*/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/**
	 * Default Weapon
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Default Weapon")
	TSubclassOf<AWeapon> DefaultWeapon;
};
