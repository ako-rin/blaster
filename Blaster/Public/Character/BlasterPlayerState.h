// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterType/Team.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnRep_Score() override;
	
	UFUNCTION()
	void OnRep_Defeats();
	
	UFUNCTION()
	void OnRep_Team();
	
	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	
	FORCEINLINE int32 GetDefeats() const {return Defeats;}
	FORCEINLINE void SetDefeats(int32 Defeat) {Defeats = Defeat;}
	
	FORCEINLINE ETeam GetTeam() const {return Team;}
	void SetTeam(ETeam TeamToSet);

private:
	UPROPERTY() // 及时垃圾回收
	class ABlasterCharacter* BlasterCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterController;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
	
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;
};
