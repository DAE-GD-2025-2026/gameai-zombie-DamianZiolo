// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "AIWorldMemoryTypes.h"
#include "StudentPerceptor.generated.h"



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZIOLODAMIANZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStudentPerceptor();
	
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	
	const TArray<FKnownZombie>& GetKnownZombies();
	const TArray<FKnownItem>& GetKnownItems();
	const TArray<FKnownHouse>& GetKnownHouses() const;
	void RemoveKnownItem(AActor* ItemActor);
	void MarkHouseVisited(AActor* Actor);
private:
	UPROPERTY()
	TArray<FKnownZombie> KnownZombies;
	
	UPROPERTY()
	TArray<FKnownItem> KnownItems;
	
	UPROPERTY()
	TArray<FKnownHouse> KnownHouses;
	

	void HandleHousePerception(AActor* Actor);
	bool IsHouse(AActor* Actor) const;

	
	void HandleZombiePerception(AActor* Actor);
	bool IsZombie(AActor* Actor) const;
	void CleanupExpiredZombies();
	
	void HandleItemPerception(AActor* Actor);
	bool IsItem(AActor* Actor) const;
	void CleanupKnownItems();
	
	


	APawn* GetControlledPawn() const;
	const float MemoryDuration = 5.f;
};
