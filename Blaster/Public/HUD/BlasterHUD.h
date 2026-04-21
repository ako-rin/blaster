// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UAnnouncement;

USTRUCT(BlueprintType)
struct  FHUDPackage
{
	GENERATED_BODY()
	
public:
	class UTexture2D* CrosshairCenter;
	class UTexture2D* CrosshairsLeft;
	class UTexture2D* CrosshairsRight;
	class UTexture2D* CrosshairsTop;
	class UTexture2D* CrosshairsBottom;
	float CrosshairSpread;
	FLinearColor CrosshairColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

protected:

	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package){ HUDPackage = Package; }

	FORCEINLINE UAnnouncement* GetAnnouncement() const {return Announcement;}
	
	class UCharacterOverlay* GetCharacterOverlay() const; 

	void AddCharacterOverlay(); // 将用户控件添加至 HUD 上
	void AddAnnouncement();
	
	void AddElimAnnouncement(FString AttackerName, FString VictimName);

private:

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FLinearColor CrosshairColor);
	
	UFUNCTION()
	void ElimAnnouncementTimerFinished(class UElimAnnouncement* MsgToRemove);


private:

	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrossshairSpreadMax = 16.f;
	
	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 2.5f;
	
	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;
	
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass; // get blueprint instance

	UPROPERTY()
	UAnnouncement* Announcement; // point to blueprint
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;
	
	UPROPERTY()
	APlayerController* OwningPlayer;
	
};
