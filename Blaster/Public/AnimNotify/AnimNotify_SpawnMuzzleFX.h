// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnMuzzleFX.generated.h"

class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class BLASTER_API UAnimNotify_SpawnMuzzleFX : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual FString GetNotifyName_Implementation() const override {return TEXT("Spawn Muzzle FX");}

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	TObjectPtr<UNiagaraSystem> SystemTemplate = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	FName AttachSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	float GlobalScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	bool bUseBulletShell = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	FVector BulletShellOfs = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MuzzleFX")
	float ShellEjectAngle = 0.f;
};
