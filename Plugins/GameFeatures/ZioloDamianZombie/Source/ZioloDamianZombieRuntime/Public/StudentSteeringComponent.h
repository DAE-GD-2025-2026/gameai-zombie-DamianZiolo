// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIWorldMemoryTypes.h"
#include "StudentSteeringComponent.generated.h"

class UStudentPerceptor;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZIOLODAMIANZOMBIERUNTIME_API UStudentSteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStudentSteeringComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float ThreatRange = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float WanderOffsetDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float WanderRadius = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float MaxWanderAngleChange = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float RotationSpeed = 6.0f;
private:
	float WanderAngle = 0.0f;
	
	enum class ESteeringMode
	{
		Wander,
		Flee,
		SeekHouse,
		SeekItem
	};

	ESteeringMode CurrentMode = ESteeringMode::Wander;
	bool HasNearbyThreat(const TArray<FKnownZombie>& KnownZombies, const FVector& OwnerLocation) const;
	FVector CalculateFleeDirection(const TArray<FKnownZombie>& KnownZombies, const FVector& OwnerLocation) const;
	FVector CalculateWanderDirection(APawn* OwnerPawn, float DeltaTime);
	void RotateTowardsMovement(APawn* OwnerPawn, const FVector& Direction, float DeltaTime) const;
	
	bool HasKnownUnvisitedHouse(const TArray<FKnownHouse>& KnownHouses) const;
	FVector CalculateSeekHouseDirection(APawn* OwnerPawn, UStudentPerceptor* Perceptor, const TArray<FKnownHouse>& KnownHouses);
	
	UPROPERTY()
	TArray<FVector> CurrentPath;

	int32 CurrentPathIndex = 0;

	UPROPERTY()
	TObjectPtr<AActor> CurrentHouseTarget = nullptr;
	UPROPERTY()
	TObjectPtr<AActor> CurrentItemTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Path Following")
	float WaypointReachDistance = 100.0f;
	
	void BuildPathToLocation(APawn* OwnerPawn, const FVector& TargetLocation);
	FVector CalculateFollowPathDirection(APawn* OwnerPawn);
	
	bool HasKnownItem(const TArray<FKnownItem>& KnownItems) const;
	FVector CalculateSeekItemDirection(APawn* OwnerPawn, UStudentPerceptor* Perceptor, const TArray<FKnownItem>& KnownItems);
	bool TryPickupItem(APawn* OwnerPawn, UStudentPerceptor* Perceptor, AActor* ItemActor);
};
