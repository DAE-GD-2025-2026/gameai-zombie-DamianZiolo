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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	UPROPERTY()
	TArray<FKnownZombie> KnownZombies;
	
	UPROPERTY()
	TArray<FKnownItem> KnowItems;
	
	UPROPERTY()
	TArray<FKnownHouse> KnownHouses;
	
	void HandleZombiePerception(AActor* Actor);
	void CleanupExpiredZombies();

	bool IsZombie(AActor* Actor) const;
	APawn* GetControlledPawn() const;
	void UpdateThreatBlackboard();
	
	void HandleDamageStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void DrawThreatDebug(const FVector& SurvivorLocation, const FVector& FleeDirection, const FVector& FleeLocation) const;
	
	const float MemoryDuration = 5.f;
};
