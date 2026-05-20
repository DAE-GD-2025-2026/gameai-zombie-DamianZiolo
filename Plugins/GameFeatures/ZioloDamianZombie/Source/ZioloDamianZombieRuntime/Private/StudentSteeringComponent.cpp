// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentSteeringComponent.h"
#include "StudentPerceptor.h"

// Sets default values for this component's properties
UStudentSteeringComponent::UStudentSteeringComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UStudentSteeringComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UStudentSteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}
	
	UStudentPerceptor* Perceptor = OwnerPawn->FindComponentByClass<UStudentPerceptor>();
	if (!Perceptor)
	{
		return;
	}
	
	const TArray<FKnownZombie>& KnownZombies = Perceptor->GetKnownZombies();
	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	FVector MovementDirection = FVector::ZeroVector;
	
	CurrentMode = HasNearbyThreat(KnownZombies, OwnerLocation)
		? ESteeringMode::Flee
		: ESteeringMode::Wander;

	switch (CurrentMode)
	{
	case ESteeringMode::Flee:
		MovementDirection = CalculateFleeDirection(KnownZombies, OwnerLocation);
		break;

	case ESteeringMode::Wander:
	default:
		MovementDirection = CalculateWanderDirection(OwnerPawn, DeltaTime);
		break;
	}

	MovementDirection.Z = 0.0f;

	if (MovementDirection.IsNearlyZero())
	{
		return;
	}

	MovementDirection.Normalize();

	OwnerPawn->AddMovementInput(MovementDirection, 1.0f);

	RotateTowardsMovement(OwnerPawn, MovementDirection, DeltaTime);
}

bool UStudentSteeringComponent::HasNearbyThreat(const TArray<FKnownZombie>& KnownZombies,
	const FVector& OwnerLocation) const
{
	for (const FKnownZombie& Zombie : KnownZombies)
	{
		const float Distance =
			FVector::Dist2D(
				OwnerLocation,
				Zombie.LastKnownLocation);

		if (Distance <= ThreatRange)
		{
			return true;
		}
	}

	return false;
}

FVector UStudentSteeringComponent::CalculateFleeDirection(
	const TArray<FKnownZombie>& KnownZombies,
	const FVector& OwnerLocation) const
{
	FVector FleeDirection = FVector::ZeroVector;

	for (const FKnownZombie& Zombie : KnownZombies)
	{
		FVector Away =
			OwnerLocation - Zombie.LastKnownLocation;

		Away.Z = 0.0f;

		const float Distance =
			FMath::Max(Away.Size(), 1.0f);

		if (Distance > ThreatRange)
		{
			continue;
		}

		Away.Normalize();

		// Closer zombies have stronger influence
		FleeDirection += Away / Distance;
	}

	return FleeDirection;
}

FVector UStudentSteeringComponent::CalculateWanderDirection(
	APawn* OwnerPawn,
	float DeltaTime)
{
	if (!OwnerPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector AgentLocation =
		OwnerPawn->GetActorLocation();

	const float YawRad =
		FMath::DegreesToRadians(
			OwnerPawn->GetActorRotation().Yaw);

	const FVector Forward = FVector(
		FMath::Cos(YawRad),
		FMath::Sin(YawRad),
		0.0f
	);

	const FVector CircleCenter =
		AgentLocation + Forward * WanderOffsetDistance;

	const float DeltaAngle =
		FMath::FRandRange(
			-MaxWanderAngleChange,
			MaxWanderAngleChange);

	WanderAngle += DeltaAngle;

	if (WanderAngle > PI)
	{
		WanderAngle -= 2.0f * PI;
	}

	if (WanderAngle < -PI)
	{
		WanderAngle += 2.0f * PI;
	}

	const FVector OffsetOnCircle = FVector(
		FMath::Cos(WanderAngle),
		FMath::Sin(WanderAngle),
		0.0f
	);

	const FVector WanderTarget =
		CircleCenter + OffsetOnCircle * WanderRadius;

	FVector Direction =
		WanderTarget - AgentLocation;

	Direction.Z = 0.0f;

	return Direction;
}

void UStudentSteeringComponent::RotateTowardsMovement(
	APawn* OwnerPawn,
	const FVector& Direction,
	float DeltaTime) const
{
	if (!OwnerPawn)
	{
		return;
	}

	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRotation =
		OwnerPawn->GetActorRotation();

	const FRotator TargetRotation =
		Direction.Rotation();

	const FRotator NewRotation =
		FMath::RInterpTo(
			CurrentRotation,
			TargetRotation,
			DeltaTime,
			RotationSpeed);

	OwnerPawn->SetActorRotation(NewRotation);
}

