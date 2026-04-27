// Fill out your copyright notice in the Description page of Project Settings.


#include "Gamemode/CaptureFlagGameMode.h"
#include "Weapon/Flag.h"
#include "CaptureTheFlag/FlagZone.h"
#include "GameState/BlasterGameState.h"
#include "Character/BlasterCharacter.h"

void ACaptureFlagGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter,
                                            class ABlasterPlayerController* VictimController,
                                            ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);
	
	if (ElimmedCharacter)
	{
		TArray<AActor*> AttachedActors;
		ElimmedCharacter->GetAttachedActors(AttachedActors);
		for (const auto AttachedActor : AttachedActors)
		{
			if (AFlag* Flag = Cast<AFlag>(AttachedActor))
			{
				if (Flag->GetAttachParentSocketName() == FName("FlagSocket"))
				{
					Flag->ResetFlag(); 
					break;
				}
			}
		}
	}
}

void ACaptureFlagGameMode::FlagCaptured(class AFlag* Flag, class AFlagZone* Zone)
{
	bool bValidCapture = Flag->GetTeam() != Zone->Team;
	if (!bValidCapture)
	{
		return;
	}
	
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(GameState);
	if (BGameState)
	{
		if (Zone->Team == ETeam::ET_BlueTeam)
		{
			BGameState->BlueTeamScores();
		}
		if (Zone->Team == ETeam::ET_RedTeam)
		{
			BGameState->RedTeamScores();
		}
	}
}
