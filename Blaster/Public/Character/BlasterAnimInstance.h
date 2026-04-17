// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterType/TurningPlace.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	
	void PlayFireMontage();
	void PlayReloadMontage();
	void PlayHitReactMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();

	void PlayShotgunReloadEndMontage();
	
	void PlayEquipMontage();

protected:

	UFUNCTION()
	void AnimNotify_FootStepL();
	
	UFUNCTION()
	void AnimNotify_FootStepR();

	UFUNCTION()
	void AnimNotify_FinishReloading();

	UFUNCTION()
	void AnimNotify_Shell();

	UFUNCTION()
	void AnimNotify_FinishedGrenadeThrow();

	UFUNCTION()
	void AnimNotify_GrenadeLaunch();
	
	UFUNCTION()
	void AnimNotify_FinishEquipWeapon();
	
	UFUNCTION()
	void AnimNotify_FinishAttachWeapon();

private:
	void Debug_DrawMuzzleLine(const bool& bDrawMuzzleLine) const;

	
protected:

	UPROPERTY(BlueprintReadOnly, Category = Character)
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float SpeedXY;
	
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float SpeedZ;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsCrouched;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsAiming;
	
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsWalking;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bIsLocal; // bIsLocallyControlled

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bRotateRootBone;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bElimmed;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float Lean;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	float AO_Pitch;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bUseFABRIK;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bTransformRightHand;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	bool bUseAimOffsets;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Movement)
	ETurningInPlace TurningInPlace;
	
	UPROPERTY(BlueprintReadOnly, Category = Movement)
	FRotator RightHandRotation;

	UPROPERTY()
	class AWeapon* EquippedWeapon;

	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;


private:

	/**
	 * Montages
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* ThrowGrenadeMontage;
	
	UPROPERTY(EditAnywhere, Category = "Combat|Montage")
	UAnimMontage* EquipMontage;
	
	/**
	 * Sounds
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Sound")
	TObjectPtr<class USoundCue> FootStepL;

	UPROPERTY(EditAnywhere, Category = "Combat|Sound")
	TObjectPtr<class USoundCue> FootStepR;


};
