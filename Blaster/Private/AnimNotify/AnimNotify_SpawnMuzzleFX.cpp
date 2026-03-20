// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AnimNotify_SpawnMuzzleFX.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

void UAnimNotify_SpawnMuzzleFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
									   const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !SystemTemplate) return;

	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		SystemTemplate,
		MeshComp,
		AttachSocketName,
		LocationOffset,
		RotationOffset,
		EAttachLocation::SnapToTarget,
		true // bAutoDestroy
	);

	if (Comp)
	{
		// 这里的名字必须和 Niagara 里真实参数名一致
		Comp->SetFloatParameter(TEXT("User.Global Scale"), GlobalScale);
		Comp->SetBoolParameter(TEXT("User.Use Bullet Shell"), bUseBulletShell);
		Comp->SetVectorParameter(TEXT("User.Bullet Shell Ofs"), BulletShellOfs);
		Comp->SetFloatParameter(TEXT("User.Shell Eject Angle"), ShellEjectAngle);}
}