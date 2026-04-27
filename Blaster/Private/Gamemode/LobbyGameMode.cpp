// Fill out your copyright notice in the Description page of Project Settings.


#include "Gamemode/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		check(Subsystem);
		
		int32 NumberOfPlayer = GameState.Get()->PlayerArray.Num();
		int32 TargetPlayers = Subsystem->DesiredNumPublicConnections;
		if (TargetPlayers < 2)
		{
			TargetPlayers = 2;
		}
		if (NumberOfPlayer >= TargetPlayers)
		{
			bUseSeamlessTravel = true;
			
			FString MatchType = Subsystem->DesiredMatchType;
		
			if (MatchType.IsEmpty())
			{
				MatchType = TEXT("SingleCombat"); 
			}

			FString GameModePath = TEXT("");
			if (MatchType == TEXT("SingleCombat"))
			{
				GameModePath = TEXT("/Game/Blueprints/Gamemode/BP_BlasterGameMode.BP_BlasterGameMode_C");
			}
			else if (MatchType == TEXT("TeamsMode"))
			{
				GameModePath = TEXT("/Game/Blueprints/Gamemode/BP_TeamGameMode.BP_TeamGameMode_C");
			}
			else if (MatchType == TEXT("CaptureTheFlag"))
			{
				GameModePath = TEXT("/Game/Blueprints/Gamemode/BP_CaptureFlagGameMode.BP_CaptureFlagGameMode_C");
			}

			FString MapPath = TEXT("/Game/Maps/TEST");
			FString FullURL = FString::Printf(TEXT("%s?game=%s?listen"), *MapPath, *GameModePath);

			UE_LOG(LogTemp, Warning, TEXT("Starting Seamless Travel with URL: %s"), *FullURL);

			GetWorld()->ServerTravel(FullURL);
		}
	}
}

