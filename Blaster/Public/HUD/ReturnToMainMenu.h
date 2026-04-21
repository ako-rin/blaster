// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual bool Initialize() override;
	void MenuSetup();
	void MenuTearDown();
	
private:
		
	UFUNCTION()
	void ReturnButtonClicked();
	
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	
	UFUNCTION()
	void OnPlayerLeftGame();
	
private:
	UPROPERTY(meta = (Bindwidget))
	class UButton* ReturnButton;
	
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	UPROPERTY()
	APlayerController* PlayerController;

};
