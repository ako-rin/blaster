// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Weapon/WeaponTypes.h"
#include "BlasterComponent/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:

	void EquipWeapon(class AWeapon* WeaponToEquip);
	void SwapWeapon();
	bool ShouldSwapWeapons();
	
	void UpdateMoveSpeed();

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	UFUNCTION()
	void OnRep_Aiming();

	UFUNCTION()
	void OnRep_Walking();

	UFUNCTION()
	void OnRep_CarriedAmmo();

	UFUNCTION()
	void OnRep_CombatState();

	FORCEINLINE bool IsAiming() const {return bAiming;}
	FORCEINLINE bool IsWalking() const {return bWalking;}

	FORCEINLINE FVector GetHitTarget() const {return HitTarget;}
	FORCEINLINE ECombatState GetCombatState() const {return CombatState;}

	FORCEINLINE int32 GetCarriedAmmo() const {return CarriedAmmo;}
	
	FORCEINLINE float GetGrenadeMaxCooldownTime() const {return GrenadeCooldownTime;}
	FORCEINLINE float GetLastThrowGrenadeTime() const {return LastThrowGrenadeTime;}
	
	/***
	 * MultiCast 广播给所有角色执行逻辑
	 */
	UFUNCTION(NetMulticast, Unreliable)
	void MultiCastFiring(bool bIsFiring, const FVector_NetQuantize& TraceHitResult); // Server and All Clients Call this

	/**
	 * Reload
	 */
	void FinishReloading();
	void ShotgunShellReload();

	/**
	 * Grenade
	 */
	UFUNCTION(Blueprintable)
	void ThrowGrenadeFinished();

	void ShowAttachedGrenade(bool bShowGrenade);
	void LaunchGrenade(); // Notify Call

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	/**
	 * Pickup
	 */
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	/**
	 * Weapon
	 */
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	int32 GetCarriedAmmoFromAmmoMap();
	
	
protected:
	void DoAiming(bool bIsAiming);
	void DoWalking(bool bIsWalking);
	void Fire();
	void DoFiring(bool bIsFiring);
	void DoReloading();
	void DoThrowGrenade();
private:

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetWalking(bool bIsWalking);
	
	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerShotGunReload();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	/**
	 * Handle Functions
	 * Used by Server Functions and OnRep Functions
	 */
	void HandleReload();

	/**
	 * 
	 * @param bIsFiring Firing: true, No Firing: false
	 * @param TraceHitResult 量化后的 FVector，用于网络传输，FVector_NetQuantize 是 FVector 的子结构体
	 */
	UFUNCTION(Server, Reliable)
	void ServerFiring(bool bIsFiring, const FVector_NetQuantize& TraceHitResult);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	/**
	 * Execute Crosshair-related logic
	 */
	void SetHUDCrosshairs(float DeltaTime);

	/**
	 * FOV
	 */
	void InterpFOV(float DeltaTime);

	/**
	 * Automatic Fire
	 */
	void StartFireTimer();
	void FireTimerFinished();
	bool CanFire();

	/**
	 * Ammo
	 */
	void InitializeCarriedAmmo();
	int32 AmountToReload();
	void UpdateAmmoValues();
	void UpdateShotGunAmmoValues();

	/**
	 * Grenade
	 */
	void StartGrenadeCooldownTimer();
	void GrenadeCooldownFinished();

	/**
	 * Refactor Functions
	 */
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquippedWeaponSound(AWeapon* WeaponToEquip);
	void ReloadEmptyWeapon();
		
public:
	UPROPERTY()
	ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	class ABlasterHUD* HUD;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;
	
private:

	// Needn't to replicate, Button pressed
	bool bFiring;
	
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming;

	UPROPERTY(ReplicatedUsing = OnRep_Walking)
	bool bWalking;

	// Tick updated
	FVector HitTarget;
	
	/**
	 * HUD and Crosshairs
	 */

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairFireFactor;

	FHUDPackage HUDPackage;

	/**
	 * Aiming and FOV
	 */

	// Filed of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = "Combat|ZoomFOV")
	float ZoomedFOV = 45.f;
	
	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = "Combat|ZoomFOV")
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = "Combat|TraceStartOffset")
	float TraceStartBias{10.f}; // 射线检测起点偏移，防止检测到自己

	/**
	 * Automatic Fire
	 */
	
	FTimerHandle FireTimer;
	bool bCanFire = true; // 防止连击多点快速开火，必须等计时器结束才能再次开火

	/**
	 * Ammo
	 */
	/**
	 * 用于显示携带的弹药数
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UPROPERTY(EditAnywhere, DisplayName = "Carried AssaultRifle Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingARAmmo = 180;

	UPROPERTY(EditAnywhere, DisplayName = "Carried Pistol Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingPistolAmmo = 90;

	UPROPERTY(EditAnywhere, DisplayName = "Carried SubmachineGun Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingSubmachineGunAmmo = 150;
	
	UPROPERTY(EditAnywhere, DisplayName = "Carried Rocket Launcher Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingRocketLauncherAmmo = 1;
	
	UPROPERTY(EditAnywhere, DisplayName = "Carried Shotgun Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingShotgunAmmo = 21;
	
	UPROPERTY(EditAnywhere, DisplayName = "Carried Shotgun Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingSniperAmmo = 20;
	
	UPROPERTY(EditAnywhere, DisplayName = "Carried Shotgun Starting Ammo", Category = "Combat|Starting Ammo")
	int32 StartingGrenadeLauncherAmmo = 10;
	
	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere, DisplayName = "Max Carried Ammo", Category = "Combat|Starting Ammo")
	int32 MaxCarriedAmmo = 999;

	/**
	 * Combat State
	 */
	UPROPERTY(ReplicatedUsing=OnRep_CombatState, VisibleAnywhere)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	/**
	 * Grenade
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Grenade")
	TSubclassOf<class AProjectile> GrenadeClass;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Grenade")
	float GrenadeCooldownTime = 10.f;
	
	float LastThrowGrenadeTime = 0.f;

	FTimerHandle GrenadeTimer;
	bool bCanThrowGrenade = true;

	/**
	 * Movement speed factor
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Movement")
	float WalkingFactor = 0.4f;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Movement")
	float AimingFactor = 0.3f;
};
