// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Weapon/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// if (!HasAuthority()) return; // Multicast call this, so only for server
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (!InstigatorPawn)
	{
		/**
		 * 必须判断 InstigatorPawn 是否有效
		 * 原因在于由于 RPC 与属性同步的竞态条件，不能保证通过 RPC 传播过后调用的 Fire 是在角色淘汰前还是淘汰后
		 * 如果不进行判断，则服务端会正常执行所有的逻辑，但是通过 Multicast 让客户端调用 Fire 时，
		 * 服务端的我早已被淘汰，其武器已丢在地上，就是当前的 ProjectileWeapon 
		 * 而客户端中的我才开火，而且又由于 RPC 顺序的不确定性，即使服务端被淘汰立刻就设置 EquippedWeapon = nullptr
		 * 但是在客户端中还没来及复制更新，就导致客户端中的 EquippedWeapon 还有值，因此依旧能够触发 Fire 函数
		 * 此时在 Fire 函数中获取到的 GetOwner 就是 nullptr ，最后触发异常
		 */
		return;
	}
	UWorld* World = GetWorld();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("Muzzle"));
	if (World && MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // Socket -> Target
		FRotator TargetRotation = ToTarget.Rotation();
		
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = GetOwner();
		SpawnParameters.Instigator = InstigatorPawn;
	
		AProjectile* SpawnProjectile = nullptr;
		
		// 从武器中设置射出的子弹是否为倒带子弹
		// - 倒带子弹：不进行复制，且不直接造成伤害
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // Server
			{
				if (InstigatorPawn->IsLocallyControlled()) // Server, host - use replicated projectile
				{
					SpawnProjectile = World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnProjectile->bUseServerSideRewind = false;
					SpawnProjectile->Damage = Damage; // for rifle weapon
					SpawnProjectile->HeadDamage = HeadDamage;
				}
				else // Server, not locally controlled - spawn non-replicated projectile, SSR (Server Side Rewind)
				{
					SpawnProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnProjectile->bUseServerSideRewind = true; // 服务器上的倒带子弹不直接施加伤害（由对应客户端发送 RPC）
				}
			}
			else // Client, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use SSR
				{
					SpawnProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnProjectile->bUseServerSideRewind = true;
					SpawnProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnProjectile->InitialVelocity = SpawnProjectile->GetActorForwardVector() * SpawnProjectile->InitialSpeed;
				}
				else // client, no locally controlled - spawn non-replicated projectile, no SSR
				{
					SpawnProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParameters);
					SpawnProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // weapon not using SSR
		{
			if (InstigatorPawn->HasAuthority())
			{
				SpawnProjectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParameters
				);
				SpawnProjectile->Damage = Damage;
				SpawnProjectile->HeadDamage = HeadDamage;
			}
		}
	}
}
