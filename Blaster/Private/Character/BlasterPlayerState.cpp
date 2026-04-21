// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "Character/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDDefeats(GetDefeats());
		}
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterCharacter->UpdateTeamOutline();
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	SetDefeats(GetDefeats() + DefeatsAmount);

	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->GetController()) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDDefeats(GetDefeats());
		}
	}
}

void ABlasterPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;
	
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterCharacter->UpdateTeamOutline();
	}
}
