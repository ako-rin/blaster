// Fill out your copyright notice in the Description page of Project Settings.


#include "Gamemode/LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayer = GameState.Get()->PlayerArray.Num();

	if (NumberOfPlayer == 2)
	{
		if (UWorld* World = GetWorld())
		{
			bUseSeamlessTravel = true;
			if (!World->ServerTravel(FString("/Game/Maps/TEST?listen")))
			{
				UE_LOG(LogTemp, Warning, TEXT("Can't Travel to BlasterMap"));				
			}
		}
	}
}
