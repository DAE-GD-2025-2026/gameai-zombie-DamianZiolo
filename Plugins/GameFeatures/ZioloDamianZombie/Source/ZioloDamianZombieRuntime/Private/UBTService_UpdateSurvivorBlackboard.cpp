#include "UBTService_UpdateSurvivorBlackboard.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "StudentPerceptor.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Items/BaseItem.h"

UBTService_UpdateSurvivorBlackboard::UBTService_UpdateSurvivorBlackboard()
{
	NodeName = TEXT("Update Survivor Blackboard");

	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateSurvivorBlackboard::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}

	APawn* OwnerPawn = AIController->GetPawn();
	if (!OwnerPawn)
	{
		return;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	UStudentPerceptor* Perceptor =
		OwnerPawn->FindComponentByClass<UStudentPerceptor>();

	if (!Perceptor)
	{
		return;
	}

	const TArray<FKnownZombie>& KnownZombies = Perceptor->GetKnownZombies();
	const TArray<FKnownHouse>& KnownHouses = Perceptor->GetKnownHouses();
	const TArray<FKnownItem>& KnownItems = Perceptor->GetKnownItems();

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();

	Blackboard->SetValueAsBool(TEXT("HasThreat"), HasNearbyThreat(KnownZombies, OwnerLocation));
	Blackboard->SetValueAsBool(TEXT("HasKnownHouse"), HasKnownUnvisitedHouse(KnownHouses));

	bool bHasKnownWeapon = false;
	bool bHasKnownFood = false;
	bool bHasKnownMedkit = false;

	for (const FKnownItem& KnownItem : KnownItems)
	{
		if (!IsValid(KnownItem.Actor))
		{
			continue;
		}

		ABaseItem* Item = Cast<ABaseItem>(KnownItem.Actor);
		if (!Item || Item->IsHidden())
		{
			continue;
		}

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
	Blackboard->SetValueAsBool(TEXT("HasMedkit"), HasItemOfType(OwnerPawn, EItemType::Medkit));
	Blackboard->SetValueAsBool(TEXT("HasFood"), HasItemOfType(OwnerPawn, EItemType::Food));

	UHealthComponent* Health = OwnerPawn->FindComponentByClass<UHealthComponent>();
	UStaminaComponent* Stamina = OwnerPawn->FindComponentByClass<UStaminaComponent>();

	bool bHealthLow = false;
	bool bStaminaLow = false;

	if (Health)
	{
		bHealthLow =
			static_cast<float>(Health->GetHealth()) /
			static_cast<float>(Health->GetMaxHealth()) <= 0.5f;
	}

	if (Stamina)
	{
		bStaminaLow =
			Stamina->GetCurrentStamina() /
			Stamina->GetMaxStamina() <= 0.5f;
	}

	Blackboard->SetValueAsBool(TEXT("HealthLow"), bHealthLow);
	Blackboard->SetValueAsBool(TEXT("StaminaLow"), bStaminaLow);
}

//====Helpers=====
bool UBTService_UpdateSurvivorBlackboard::HasWeapon(APawn* OwnerPawn) const
{
	if (!OwnerPawn)
	{
		return false;
	}

	UInventoryComponent* Inventory =
		OwnerPawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return false;
	}

	for (ABaseItem* Item : Inventory->GetInventory())
	{
		if (!Item)
		{
			continue;
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

bool UBTService_UpdateSurvivorBlackboard::HasItemOfType(
	APawn* OwnerPawn,
	EItemType ItemType) const
{
	if (!OwnerPawn)
	{
		return false;
	}

	UInventoryComponent* Inventory =
		OwnerPawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return false;
	}

	for (ABaseItem* Item : Inventory->GetInventory())
	{
		if (!Item)
		{
			continue;
		}

		if (Item->GetItemType() == ItemType && Item->GetValue() > 0)
		{
			return true;
		}
	}

	return false;
}

bool UBTService_UpdateSurvivorBlackboard::HasNearbyThreat(
	const TArray<FKnownZombie>& KnownZombies,
	const FVector& OwnerLocation) const
{
	const float ThreatRange = 600.0f;

	for (const FKnownZombie& Zombie : KnownZombies)
	{
		const float Distance =
			FVector::Dist2D(OwnerLocation, Zombie.LastKnownLocation);

		if (Distance <= ThreatRange)
		{
			return true;
		}
	}

	return false;
}

bool UBTService_UpdateSurvivorBlackboard::HasKnownUnvisitedHouse(
	const TArray<FKnownHouse>& KnownHouses) const
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
