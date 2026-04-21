// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "TeamGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamGameMode : public ABlasterGameMode
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Called after a successful login. 
	 * This is the first place it is safe to call replicated functions (RPC) on the PlayerController. 
	 * @param NewPlayer 
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/**
	 * Called when a Controller with a PlayerState leaves the game or is destroyed
	 * @param Exiting 
	 */
	virtual void Logout(AController* Exiting) override;
	
protected:
	virtual void HandleMatchHasStarted() override;
};
