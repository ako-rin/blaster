// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "Weapon.generated.h"

class USphereComponent;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "MAX")
};

UENUM()
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),
	
	EFT_MAX UMETA(DisplayName = "Default MAX")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FORCEINLINE bool IsUseScatter() const {return bUseScatter;}

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_Owner() override;
	
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

public:
	
	void ShowPickupWidget(bool bShowWidget);
	
	void SetWeaponState(const EWeaponState& State);
	FORCEINLINE USphereComponent* GetAreaSphere() const {return AreaSphere;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const {return WeaponMesh;}

	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}

	FORCEINLINE float GetFireDelay() const {return FireDelay;}
	FORCEINLINE bool GetAutomaticFire() const {return bAutomaticFire;}
	FORCEINLINE EWeaponType GetWeaponType() const {return WeaponType;}

	FORCEINLINE int32 GetCurrentAmmo() const {return Ammo;}
	FORCEINLINE int32 GetMagCapacity() const {return MagCapacity;}
	FORCEINLINE USoundCue* GetEquippedSound() const {return EquippedSound;}

	FORCEINLINE void SetWeaponShouldDestroy(const bool& bEnable) {bDestroyWeapon = bEnable;}
	FORCEINLINE bool ShouldDestroyWeapon() const {return bDestroyWeapon;}
	
	FORCEINLINE EFireType GetFireType() const {return FireType;}
	
	
	virtual void Fire(const FVector& HitTarget);

	// Drop the weapon
	void Dropped();

	/**
	 * Emmo
	 */
	void SetHUDAmmo();
	void SpendRound();
	bool IsEmptyAmmo();
	bool IsFull();
	void AddAmmo(int32 AmmoToAdd);
	
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	
	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 ServerAmmo);

	/**
	 * Enable or disable custom depth
	 */
	void EnableCustomDepth(bool bEnable);

	/**
	 * Scatter
	 */
	FVector TraceEndWithScatter(const FVector& HitTarget);
	
protected:
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UFUNCTION()
	void OnRep_WeaponState();

	// 使用客户端预测，而不从服务端复制过来
	// UFUNCTION()
	// void OnRep_Ammo();


public:
	
	// The number of unprocessed server requests for ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo
	int32 Sequence = 0;
	
	/**
	 * Textures for the weapon crosshairs
	 */

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Crosshair")
	class UTexture2D* CrosshairsCenter;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Crosshair")
	class UTexture2D* CrosshairsLeft;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Crosshair")
	class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Crosshair")
	class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Crosshair")
	class UTexture2D* CrosshairsBottom;
	
protected:
	
	/**
	 * Trace end with scatter
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	float DistanceToSphere = 800.f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	float SphereRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Weapon Scatter")
	bool bUseScatter = false;
	
private:

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;

	/**
	 * Zoomed FOV while aiming
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Aimming Zoomed")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Aimming Zoomed")
	float ZoomInterpSpeed = 20.f;

	/**
	 * Automatic Fire
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bAutomaticFire = true;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float FireDelay = .15f;

	/**
	 * Ammo
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Ammo")
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Ammo")
	int32 MagCapacity = 0;

	int32 LocalAmmo = 0;

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EWeaponType WeaponType;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	EFireType FireType;

	/**
	 * Sound Effect
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Sound")
	class USoundCue* EquippedSound;

	/**
	 * Destroy
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bDestroyWeapon = false;
};
