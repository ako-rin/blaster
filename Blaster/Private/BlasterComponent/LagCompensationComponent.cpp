// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"
#include "Blaster/Blaster.h"
#include "Weapon/Shotgun.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


// Called when the game starts
void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Trace
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& ScanFrame,
	ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr || ScanFrame.HitBoxInfo.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("HitCharacter == nullptr? %d, Package is empty: %d"), HitCharacter == nullptr, ScanFrame.HitBoxInfo.IsEmpty());
		return FServerSideRewindResult{false, false};
	} 
	
	FFramePackage CurrentFrame;
	CurrentFrame.Character = ScanFrame.Character;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, ScanFrame);
	
	// ShowFramePackage(CurrentFrame, FColor::Red);
	
	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;
	Params.AddIgnoredActor(GetOwner());
	if (UWorld* World = GetWorld())
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_LagCompensation,
			Params
		);
		
		// UE_LOG(LogTemp, Warning, TEXT("Trace Result: %d"), ConfirmHitResult.bBlockingHit);
		// DrawDebugLine(World, TraceStart, TraceEnd, FColor::Orange, true, 10, 0, 3);
		
		if (ConfirmHitResult.bBlockingHit) // we hit the head, return early
		{
			ResetHitBoxes(HitCharacter, CurrentFrame);
			UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
			if (ConfirmHitResult.Component.IsValid() && ConfirmHitResult.Component == HeadBox)
			{
				return FServerSideRewindResult{ true, true };
			}
			else
			{
				return FServerSideRewindResult{ true, false };
			}
		}
	}
	// didn't hit the body
	ResetHitBoxes(HitCharacter, CurrentFrame);
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& ScanFrames,
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	// 1. 记录每个 Character 的 HitBox 位置
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : ScanFrames)
	{
		if (Frame.Character == nullptr) continue;
		
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPosition(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		CurrentFrames.Add(CurrentFrame);
	}
	
	// 2. 开始射线检测，轮询多条射线并记录结果
	
	if (UWorld* World = GetWorld())
	{
		for (auto& HitLocation : HitLocations)
		{
			FHitResult ConfirmHitResult;
			const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
			FCollisionQueryParams Params;
			Params.bTraceComplex = false;
			Params.AddIgnoredActor(GetOwner());
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_LagCompensation,
				Params
			);
			if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor()))
			{
				UBoxComponent* HeadBox = BlasterCharacter->HitCollisionBoxes[FName("head")];
				if (ConfirmHitResult.Component.IsValid() && ConfirmHitResult.Component == HeadBox)
				{
					if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
					{
						ShotgunResult.HeadShots[BlasterCharacter]++;
					}
					else
					{
						ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
					}
				}
				else
				{
					if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
					{
						ShotgunResult.BodyShots[BlasterCharacter]++;
					}
					else
					{
						ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
					}
				}
			}
		}
	}
	
	// 3. 重置 HitBox 回原来时间线
	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes(Frame.Character, Frame);
	}

	return ShotgunResult;
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& ScanFrame, ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr || ScanFrame.HitBoxInfo.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("HitCharacter == nullptr? %d, Package is empty: %d"), HitCharacter == nullptr, ScanFrame.HitBoxInfo.IsEmpty());
		return FServerSideRewindResult{false, false};
	} 
	
	FFramePackage CurrentFrame;
	CurrentFrame.Character = ScanFrame.Character;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, ScanFrame);
	
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_LagCompensation;
	PathParams.ActorsToIgnore.Add(GetOwner());
	// PathParams.DrawDebugTime = 5.f;
	// PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;
	
	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);
	
	if (PathResult.HitResult.bBlockingHit)
	{
		ResetHitBoxes(HitCharacter, CurrentFrame);
		UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
		if (PathResult.HitResult.Component.IsValid() && PathResult.HitResult.Component == HeadBox)
		{
			return FServerSideRewindResult{ true, true };
		}
		else
		{
			return FServerSideRewindResult{ true, false };
		}
	}
	ResetHitBoxes(HitCharacter, CurrentFrame);
	return FServerSideRewindResult{ false, false };
}

void ULagCompensationComponent::CacheBoxPosition(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;
	
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			// HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	MoveBoxes(HitCharacter, Package);
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			MaxRecordTime
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(class ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(
	const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}


FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}


FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr
	|| HitCharacter->GetLagCompensation() == nullptr
	|| HitCharacter->IsElimmed()
	|| HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr
	|| HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	
	if (bReturn)
	{
		return FFramePackage();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Frame Checking..."));
	
	// Frame package that we check to verify a hit
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;
	
	// Frame history of the HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	
	// 记录最后的时间比击中时间还早
	if (OldestHistoryTime > HitTime)
	{
		// too far back - too laggy to do SSR
		UE_LOG(LogTemp, Warning, TEXT("HitTime is older than the last mark"));
		return FFramePackage();
	}
	// 记录最后的时间正好是击中时
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	// 击中时间比记录最早的时间还早
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}
	
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	
	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr) break;
		
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	// 击中的时间正好在节点上
	if (Older->GetValue().Time == HitTime) // highly unlikely, but we found our frame to check
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	
	if (bShouldInterpolate)
	{
		// Interpolate between Younger and Later
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	return FrameToCheck;
}

// HitScan
void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
                                                                  const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime,
)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	
	if (!Character->GetEquippedWeapon() || !Character) return;
	AWeapon* Weapon = Character->GetEquippedWeapon();
	
	if (HitCharacter && Confirm.bHitCanFirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Weapon->GetDamage(),
			Character->Controller,
			Weapon,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (const auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetEquippedWeapon() == nullptr) continue;
		AWeapon* Weapon = Character->GetEquippedWeapon();
		
		float TotalDamage = 0.f;
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Weapon->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Weapon->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			Weapon,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100 InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	UE_LOG(LogTemp, Warning, TEXT("Confirm.bHitCanFirmed %d"), Confirm.bHitCanFirmed);
	
	if (!Character->GetEquippedWeapon() || !Character) return;
	AWeapon* Weapon = Character->GetEquippedWeapon();
	
	if (HitCharacter && Confirm.bHitCanFirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Confirm.bHeadShot ? Weapon->GetHeadShotDamage() : Weapon->GetDamage(),
			Character->Controller,
			Weapon,
			UDamageType::StaticClass()
		);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame,
                                                             const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);
	
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	InterpFramePackage.Character = YoungerFrame.Character;
	
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;
		
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];
		
		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;
		
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}
	return InterpFramePackage;
}


// Called every frame
void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;
	
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistroyLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistroyLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistroyLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		
		// ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}
