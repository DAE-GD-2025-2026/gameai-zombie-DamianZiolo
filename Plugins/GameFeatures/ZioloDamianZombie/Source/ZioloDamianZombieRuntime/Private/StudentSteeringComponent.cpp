// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentSteeringComponent.h"
#include "StudentPerceptor.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"
#include "Common/InventoryComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "DrawDebugHelpers.h"

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
	const TArray<FKnownHouse>& KnownHouses = Perceptor->GetKnownHouses();
	const TArray<FKnownItem>& KnownItems = Perceptor->GetKnownItems();
	
	UpdateBlackboardDecisionData(OwnerPawn, KnownZombies, KnownHouses, KnownItems);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			4,
			0.1f,
			FColor::Yellow,
			FString::Printf(TEXT("KnownItems: %d"), KnownItems.Num())
		);
	}
	
	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	
	const bool bHasWeapon = HasWeapon(OwnerPawn);
	
	DrawDebugString(
	GetWorld(),
	OwnerLocation + FVector(0, 0, 120.0f),
	bHasWeapon ? TEXT("HAS WEAPON") : TEXT("NO WEAPON"),
	nullptr,
	bHasWeapon ? FColor::Green : FColor::Red,
	0.0f,
	true
);

	FVector MovementDirection = FVector::ZeroVector;
	
	CurrentMode = ReadSteeringModeFromBlackboard(OwnerPawn);

	switch (CurrentMode)
	{
	case ESteeringMode::Flee:
		MovementDirection = CalculateFleeDirection(KnownZombies, OwnerLocation);
		break;

	case ESteeringMode::SeekHouse:
		MovementDirection = CalculateSeekHouseDirection(OwnerPawn, Perceptor, KnownHouses);
		break;
	case ESteeringMode::SeekItem:
		MovementDirection = CalculateSeekItemDirection(OwnerPawn, Perceptor, KnownItems);
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

	if (CurrentMode == ESteeringMode::Flee && HasWeapon(OwnerPawn))
	{
		AActor* ClosestZombie = GetClosestZombie(KnownZombies, OwnerLocation);

		if (ClosestZombie)
		{
			const FVector ZombieLocation = ClosestZombie->GetActorLocation();
			RotateTowardsTarget(OwnerPawn, ZombieLocation, DeltaTime);
			
			if (IsFacingTarget(OwnerPawn, ZombieLocation))
			{
				TryShootClosestZombie(OwnerPawn, KnownZombies);
			}
		}
	}
	else
	{
		RotateTowardsMovement(OwnerPawn, MovementDirection, DeltaTime);
	}
}

bool UStudentSteeringComponent::HasWeapon(APawn* OwnerPawn) const
{
	if (!OwnerPawn) return false;

	UInventoryComponent* Inventory = OwnerPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return false;
	
	for (ABaseItem* Item : Inventory->GetInventory())
	{
		if (!Item)
		{
			continue;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				20,
				0.1f,
				FColor::Cyan,
				FString::Printf(
					TEXT("Inv item: %s | Type: %d | Value: %d | Hidden: %s"),
					*Item->GetName(),
					static_cast<int32>(Item->GetItemType()),
					Item->GetValue(),
					Item->IsHidden() ? TEXT("true") : TEXT("false")
				)
			);
		}

		if ((Item->GetItemType() == EItemType::Pistol ||
			 Item->GetItemType() == EItemType::Shotgun) &&
			Item->GetValue() > 0)
		{
			return true;
		}
	}

	return false;
}

bool UStudentSteeringComponent::TryShootClosestZombie(APawn* OwnerPawn, const TArray<FKnownZombie>& KnownZombies)
{
	if (!OwnerPawn) return false;

	ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(OwnerPawn);
	if (!Survivor) return false;

	AActor* ClosestZombie = GetClosestZombie(KnownZombies, OwnerPawn->GetActorLocation());
	if (!ClosestZombie) return false;

	const float Distance = FVector::Dist2D(OwnerPawn->GetActorLocation(), ClosestZombie->GetActorLocation());
	if (Distance > ShootingRange) return false;

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastShotTime < ShootCooldown) return false;

	UInventoryComponent* Inventory = OwnerPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return false;

	for (int Slot = 0; Slot < Inventory->GetInventoryCapacity(); ++Slot)
	{
		const TArray<ABaseItem*>& Items = Inventory->GetInventory();

		if (!Items.IsValidIndex(Slot) || !Items[Slot]) continue;
		

		if (Items[Slot]->GetItemType() == EItemType::Pistol ||
			Items[Slot]->GetItemType() == EItemType::Shotgun)
		{
			//removing gun witouth ammo
			if (Items[Slot]->GetValue() <= 0)
			{
				Inventory->RemoveItem(Slot);
				continue;
			}
			
			if (Inventory->UseItem(Slot))
			{
				LastShotTime = CurrentTime;
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						11,
						0.5f,
						FColor::Green,
						TEXT("SHOT FIRED")
					);
				}
				return true;
			}
		}
	}

	return false;
}

AActor* UStudentSteeringComponent::GetClosestZombie(const TArray<FKnownZombie>& KnownZombies,
                                                    const FVector& OwnerLocation) const
{
	AActor* ClosestZombie = nullptr;
	float ClosestDistance = FLT_MAX;

	for (const FKnownZombie& Zombie : KnownZombies)
	{
		if (!IsValid(Zombie.Actor)) continue;

		const float Distance = FVector::Dist2D(OwnerLocation, Zombie.LastKnownLocation);

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestZombie = Zombie.Actor;
		}
	}

	return ClosestZombie;
}

void UStudentSteeringComponent::RotateTowardsTarget(APawn* OwnerPawn, const FVector& TargetLocation,
                                                    float DeltaTime) const
{
	if (!OwnerPawn) return;

	FVector Direction = TargetLocation - OwnerPawn->GetActorLocation();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero()) return;

	const FRotator TargetRotation = Direction.Rotation();

	const FRotator NewRotation = FMath::RInterpTo(
		OwnerPawn->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		RotationSpeed
	);

	OwnerPawn->SetActorRotation(NewRotation);
}

bool UStudentSteeringComponent::IsFacingTarget(APawn* OwnerPawn, const FVector& TargetLocation) const
{
	if (!OwnerPawn) return false;

	FVector ToTarget = TargetLocation - OwnerPawn->GetActorLocation();
	ToTarget.Z = 0.0f;

	if (ToTarget.IsNearlyZero()) return false;

	ToTarget.Normalize();

	FVector Forward = OwnerPawn->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	const float Dot = FVector::DotProduct(Forward, ToTarget);

	return Dot >= ShootFacingDotThreshold;
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

bool UStudentSteeringComponent::HasKnownUnvisitedHouse(const TArray<FKnownHouse>& KnownHouses) const
{
	for (const FKnownHouse& House : KnownHouses)
	{
		if (IsValid(House.Actor) && !House.bVisited)
		{
			return true;
		}
	}

	return false;
}

FVector UStudentSteeringComponent::CalculateSeekHouseDirection(
	APawn* OwnerPawn,
	UStudentPerceptor* Perceptor,
	const TArray<FKnownHouse>& KnownHouses)
{
	if (!OwnerPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	FKnownHouse BestHouse;
	float BestDistance = FLT_MAX;
	bool bFoundHouse = false;

	for (const FKnownHouse& House : KnownHouses)
	{
		if (!IsValid(House.Actor) || House.bVisited)
		{
			continue;
		}

		const float Distance = FVector::Dist2D(OwnerLocation, House.Location);

		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestHouse = House;
			bFoundHouse = true;
		}
	}

	if (!bFoundHouse)
	{
		CurrentHouseTarget = nullptr;
		CurrentPath.Empty();
		CurrentPathIndex = 0;
		return FVector::ZeroVector;
	}

	if (CurrentHouseTarget != BestHouse.Actor || CurrentPath.IsEmpty())
	{
		CurrentHouseTarget = BestHouse.Actor;
		BuildPathToLocation(OwnerPawn, BestHouse.Location);
	}

	FVector Direction = CalculateFollowPathDirection(OwnerPawn);

	if (Direction.IsNearlyZero() && CurrentHouseTarget)
	{
		Perceptor->MarkHouseVisited(CurrentHouseTarget);
		CurrentHouseTarget = nullptr;
		CurrentPath.Empty();
		CurrentPathIndex = 0;
	}

	return Direction;
}

void UStudentSteeringComponent::BuildPathToLocation(APawn* OwnerPawn, const FVector& TargetLocation)
{
	ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(OwnerPawn);
	if (!Survivor)
	{
		CurrentPath.Empty();
		CurrentPathIndex = 0;
		return;
	}

	CurrentPath = Survivor->CalculatePath(TargetLocation);
	CurrentPathIndex = CurrentPath.Num() > 1 ? 1 : 0;
}

FVector UStudentSteeringComponent::CalculateFollowPathDirection(APawn* OwnerPawn)
{
	if (!OwnerPawn || CurrentPath.IsEmpty() || !CurrentPath.IsValidIndex(CurrentPathIndex))
	{
		return FVector::ZeroVector;
	}

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	FVector CurrentWaypoint = CurrentPath[CurrentPathIndex];

	if (FVector::Dist2D(OwnerLocation, CurrentWaypoint) <= WaypointReachDistance)
	{
		CurrentPathIndex++;

		if (!CurrentPath.IsValidIndex(CurrentPathIndex))
		{
			CurrentPath.Empty();
			CurrentPathIndex = 0;
			return FVector::ZeroVector;
		}

		CurrentWaypoint = CurrentPath[CurrentPathIndex];
	}

	FVector Direction = CurrentWaypoint - OwnerLocation;
	Direction.Z = 0.0f;

	return Direction;
}

bool UStudentSteeringComponent::HasKnownItem(const TArray<FKnownItem>& KnownItems) const
{
	for (const FKnownItem& Item : KnownItems)
	{
		if (IsValid(Item.Actor) && !Item.Actor->IsHidden())
		{
			return true;
		}
	}
	return false;
}

FVector UStudentSteeringComponent::CalculateSeekItemDirection(APawn* OwnerPawn, UStudentPerceptor* Perceptor,
	const TArray<FKnownItem>& KnownItems)
{
	if (!OwnerPawn || !Perceptor) return FVector::ZeroVector;
	
	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	
	AActor* BestItem = nullptr;
	FVector BestLocation = FVector::ZeroVector;
	float BestDistance = FLT_MAX;
	
	const FName DesiredType = ReadDesiredItemTypeFromBlackboard(OwnerPawn);
	
	for (const FKnownItem& Item : KnownItems)
	{
		if (!IsValid(Item.Actor) || Item.Actor->IsHidden())
		{
			continue;
		}
		
		ABaseItem* BaseItem = Cast<ABaseItem>(Item.Actor);
		if (!BaseItem) continue;

		const EItemType ItemType = BaseItem->GetItemType();

		if (DesiredType == TEXT("Weapon"))
		{
			if (ItemType != EItemType::Pistol && ItemType != EItemType::Shotgun)
			{
				continue;
			}
		}
		else if (DesiredType == TEXT("Medkit") && ItemType != EItemType::Medkit)
		{
			continue;
		}
		else if (DesiredType == TEXT("Food") && ItemType != EItemType::Food)
		{
			continue;
		}

		const float Distance = FVector::Dist2D(OwnerLocation, Item.LastKnownLocation);

		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestItem = Item.Actor;
			BestLocation = Item.LastKnownLocation;
		}
	}

	if (!BestItem)
	{
		return FVector::ZeroVector;
	}

	if (TryPickupItem(OwnerPawn, Perceptor, BestItem))
	{
		CurrentItemTarget = nullptr;
		CurrentPath.Empty();
		CurrentPathIndex = 0;
		return CalculateWanderDirection(OwnerPawn, 0.0f);
	}

	if (CurrentItemTarget != BestItem || CurrentPath.IsEmpty())
	{
		CurrentItemTarget = BestItem;
		BuildPathToLocation(OwnerPawn, BestLocation);
	}

	FVector PathDirection = CalculateFollowPathDirection(OwnerPawn);

	if (!PathDirection.IsNearlyZero())
	{
		return PathDirection;
	}

	FVector DirectDirection = BestLocation - OwnerLocation;
	DirectDirection.Z = 0.0f;
	return DirectDirection;
	
}

bool UStudentSteeringComponent::TryPickupItem(APawn* OwnerPawn, UStudentPerceptor* Perceptor, AActor* ItemActor)
{
	if (!OwnerPawn || !Perceptor || !ItemActor) return false;
	
	ABaseItem* Item = Cast<ABaseItem>(ItemActor);
	if (!Item) return false;
	
	UInventoryComponent* Inventory = OwnerPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return false;
	
	const float Distance = FVector::Dist2D(OwnerPawn->GetActorLocation(), Item->GetActorLocation());
	
	if (Distance > Inventory->GetPickupRange())
	{
		return false;
	}
	
	for (int Slot = 0; Slot < Inventory->GetInventoryCapacity(); ++Slot)
	{
		
		if (Inventory->GrabItem(Slot, Item))
		{
			Perceptor->RemoveKnownItem(ItemActor);
			return true;
		}
	}
	
	return false;
}

UStudentSteeringComponent::ESteeringMode UStudentSteeringComponent::ReadSteeringModeFromBlackboard(
	APawn* OwnerPawn) const
{
	if (!OwnerPawn) return ESteeringMode::Wander;
	
	AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIController) return ESteeringMode::Wander;
	
	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard) return ESteeringMode::Wander;
	
	const FName ModeName = Blackboard->GetValueAsName("SteeringMode");
	
	if (ModeName == TEXT("Flee")) return ESteeringMode::Flee;
	else if (ModeName == TEXT("SeekItem")) return ESteeringMode::SeekItem;
	else if (ModeName == TEXT("SeekHouse")) return ESteeringMode::SeekHouse;
	else if (ModeName == TEXT("Wander")) return ESteeringMode::Wander;
	
	return ESteeringMode::Wander;
}

FName UStudentSteeringComponent::ReadDesiredItemTypeFromBlackboard(APawn* OwnerPawn) const
{
	if (!OwnerPawn) return NAME_None;

	AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIController) return NAME_None;

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard) return NAME_None;

	return Blackboard->GetValueAsName(TEXT("DesiredItemType"));
}

void UStudentSteeringComponent::UpdateBlackboardDecisionData(APawn* OwnerPawn, const TArray<FKnownZombie>& KnownZombies,
 const TArray<FKnownHouse>& KnownHouses, const TArray<FKnownItem>& KnownItems) const
{
	if (!OwnerPawn) return;

	AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIController) return;

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard) return;

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	Blackboard->SetValueAsBool(TEXT("HasThreat"), HasNearbyThreat(KnownZombies, OwnerLocation));
	Blackboard->SetValueAsBool(TEXT("HasKnownHouse"), HasKnownUnvisitedHouse(KnownHouses));

	bool bHasKnownWeapon = false;
	bool bHasKnownFood = false;
	bool bHasKnownMedkit = false;

	for (const FKnownItem& KnownItem : KnownItems)
	{
		ABaseItem* Item = Cast<ABaseItem>(KnownItem.Actor);
		if (!Item || Item->IsHidden()) continue;

		switch (Item->GetItemType())
		{
		case EItemType::Pistol:
		case EItemType::Shotgun:
			bHasKnownWeapon = true;
			break;

		case EItemType::Food:
			bHasKnownFood = true;
			break;

		case EItemType::Medkit:
			bHasKnownMedkit = true;
			break;

		default:
			break;
		}
	}

	Blackboard->SetValueAsBool(TEXT("HasKnownWeapon"), bHasKnownWeapon);
	Blackboard->SetValueAsBool(TEXT("HasKnownFood"), bHasKnownFood);
	Blackboard->SetValueAsBool(TEXT("HasKnownMedkit"), bHasKnownMedkit);
	Blackboard->SetValueAsBool(TEXT("HasWeapon"), HasWeapon(OwnerPawn));
}
