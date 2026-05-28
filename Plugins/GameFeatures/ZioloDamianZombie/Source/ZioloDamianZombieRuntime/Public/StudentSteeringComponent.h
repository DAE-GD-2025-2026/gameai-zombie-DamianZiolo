#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIWorldMemoryTypes.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Items/ItemType.h"
#include "StudentSteeringComponent.generated.h"

class UStudentPerceptor;
class ABaseItem;
class UInventoryComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ZIOLODAMIANZOMBIERUNTIME_API UStudentSteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Unreal lifecycle
	UStudentSteeringComponent();

protected:
	// Called when the component starts
	virtual void BeginPlay() override;

public:
	// Main steering update
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Steering tuning
	UPROPERTY(EditAnywhere, Category = "Steering")
	float ThreatRange = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float WanderOffsetDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float WanderRadius = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float MaxWanderAngleChange = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Steering")
	float RotationSpeed = 6.0f;

	// Combat tuning
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootingRange = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootCooldown = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootFacingDotThreshold = 0.85f;

	float LastShotTime = -999.0f;

	// Combat helpers
	bool HasWeapon(APawn* OwnerPawn) const;
	bool TryShootClosestZombie(APawn* OwnerPawn, const TArray<FKnownZombie>& KnownZombies);
	AActor* GetClosestZombie(const TArray<FKnownZombie>& KnownZombies, const FVector& OwnerLocation) const;
	void RotateTowardsTarget(APawn* OwnerPawn, const FVector& TargetLocation, float DeltaTime) const;
	bool IsFacingTarget(APawn* OwnerPawn, const FVector& TargetLocation) const;

	// House exit state
	UPROPERTY()
	TObjectPtr<AActor> LastVisitedHouse = nullptr;
	UPROPERTY()
	FVector ExitHouseTarget = FVector::ZeroVector;
	bool bHasExitHouseTarget = false;
	
	bool IsInsideHouseBounds(APawn* OwnerPawn, AActor* HouseActor) const;
	FVector PickRandomExitLocationNearHouse(AActor* HouseActor) const;
	void ClearHouseExitState();

	UPROPERTY(EditAnywhere, Category = "House")
	float ExitHouseDistance = 800.0f;

private:
	// Steering mode state
	enum class ESteeringMode
	{
		Wander,
		Flee,
		SeekHouse,
		SeekItem,
		SearchItem,
		ExitHouse
	};

	ESteeringMode CurrentMode = ESteeringMode::Wander;
	float WanderAngle = 0.0f;

	// Basic steering behaviors
	FVector CalculateFleeDirection(const TArray<FKnownZombie>& KnownZombies, const FVector& OwnerLocation) const;
	FVector CalculateWanderDirection(APawn* OwnerPawn);
	void RotateTowardsMovement(APawn* OwnerPawn, const FVector& Direction, float DeltaTime) const;

	// House search / exploration
	bool HasKnownUnvisitedHouse(const TArray<FKnownHouse>& KnownHouses) const;
	FVector CalculateSeekHouseDirection(APawn* OwnerPawn, UStudentPerceptor* Perceptor, const TArray<FKnownHouse>& KnownHouses);
	FVector CalculateExitHouseDirection(APawn* OwnerPawn);

	// Item search / pickup
	bool HasKnownItem(const TArray<FKnownItem>& KnownItems) const;
	bool HasKnownDesiredItem(const TArray<FKnownItem>& KnownItems, FName DesiredItemType) const;
	bool DoesItemMatchDesiredType(ABaseItem* Item, FName DesiredItemType) const;

	FVector CalculateSeekItemDirection(APawn* OwnerPawn, UStudentPerceptor* Perceptor, const TArray<FKnownItem>& KnownItems);
	FVector CalculateSearchItemDirection(
		APawn* OwnerPawn,
		UStudentPerceptor* Perceptor,
		const TArray<FKnownItem>& KnownItems,
		const TArray<FKnownHouse>& KnownHouses
	);

	bool TryPickupItem(APawn* OwnerPawn, UStudentPerceptor* Perceptor, AActor* ItemActor);
	void TryPickupNearbyItems(
		APawn* OwnerPawn,
		UStudentPerceptor* Perceptor,
		const TArray<FKnownItem>& KnownItems
	);

	// Inventory helpers
	bool TryUseItemOfType(APawn* OwnerPawn, EItemType ItemType);
	bool MakeRoomForImportantItem(UInventoryComponent* Inventory, ABaseItem* NewItem);
	bool IsInventoryFull(UInventoryComponent* Inventory) const;

	// Path following
	UPROPERTY()
	TArray<FVector> CurrentPath;

	int32 CurrentPathIndex = 0;

	UPROPERTY()
	TObjectPtr<AActor> CurrentHouseTarget = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> CurrentItemTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Path Following")
	float WaypointReachDistance = 60.0f;

	void BuildPathToLocation(APawn* OwnerPawn, const FVector& TargetLocation);
	FVector CalculateFollowPathDirection(APawn* OwnerPawn);

	// Blackboard communication
	ESteeringMode ReadSteeringModeFromBlackboard(APawn* OwnerPawn) const;
	FName ReadDesiredItemTypeFromBlackboard(APawn* OwnerPawn) const;
	
	//Debugs
	FString GetSteeringModeName() const;
	void DrawSteeringDebug(
		APawn* OwnerPawn,
		const TArray<FKnownItem>& KnownItems,
		const TArray<FKnownHouse>& KnownHouses
	) const;
};