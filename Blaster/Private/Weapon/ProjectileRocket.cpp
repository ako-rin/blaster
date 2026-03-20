// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Weapon/RocketMovementComponent.h"


AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket Movement Component"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		GetCollisionBox()->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}

	SpawnTrailSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileAudioLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::Type::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency* )nullptr,
			false
		);
	}
}

void AProjectileRocket::Destroyed()
{
	// 为了延时显示火箭弹尾迹，因此不直接进行销毁，而放在计时器结束后再销毁
	// 因此这里重写父类并留空
}


void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& HitResult)
{

	if (OtherActor == GetOwner())
	{
		return;
	}
	
	ExplodeDamage();

	if (GetProjectileImpact())
	{
		UNiagaraComponent* Impact = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			GetProjectileImpact(),
			GetActorLocation()
		);
	}
	if (GetImpactSound())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			GetImpactSound(),
			GetActorLocation()
		);
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	
	if (GetCollisionBox())
	{
		GetCollisionBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	DeactivateTrailSystem();

	if (ProjectileAudioLoopComponent && ProjectileAudioLoopComponent->IsPlaying())
	{
		ProjectileAudioLoopComponent->Stop();
	}
}

