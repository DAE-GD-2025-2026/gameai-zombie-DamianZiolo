#include "BTTask_ShootClosestZombie.h"

#include "AIController.h"
#include "StudentPerceptor.h"
#include "AIWorldMemoryTypes.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_ShootClosestZombie::UBTTask_ShootClosestZombie()
{
	NodeName = TEXT("Shoot Closest Zombie");
}

EBTNodeResult::Type UBTTask_ShootClosestZombie::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* OwnerPawn = AIController->GetPawn();
	if (!OwnerPawn)
	{
		return EBTNodeResult::Failed;
	}

	UStudentPerceptor* Perceptor =
		OwnerPawn->FindComponentByClass<UStudentPerceptor>();

	if (!Perceptor)
	{
		return EBTNodeResult::Failed;
	}

	const TArray<FKnownZombie>& KnownZombies = Perceptor->GetKnownZombies();

	AActor* ClosestZombie =
		GetClosestZombie(KnownZombies, OwnerPawn->GetActorLocation());

	if (!ClosestZombie)
	{
		return EBTNodeResult::Failed;
	}

	const float Distance =
		FVector::Dist2D(
			OwnerPawn->GetActorLocation(),
			ClosestZombie->GetActorLocation()
		);

	if (Distance > ShootingRange)
	{
		return EBTNodeResult::Failed;
	}

	FVector Direction = ClosestZombie->GetActorLocation() - OwnerPawn->GetActorLocation();
	Direction.Z = 0.0f;

	if (!Direction.IsNearlyZero())
	{
		OwnerPawn->SetActorRotation(Direction.Rotation());
	}
	
	if (!IsFacingTarget(OwnerPawn, ClosestZombie->GetActorLocation()))
	{
		return EBTNodeResult::Failed;
	}

	if (TryUseWeapon(OwnerPawn))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				11,
				0.5f,
				FColor::Green,
				TEXT("BT SHOT FIRED")
			);
		}

		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

AActor* UBTTask_ShootClosestZombie::GetClosestZombie(
	const TArray<FKnownZombie>& KnownZombies,
	const FVector& OwnerLocation) const
{
	AActor* ClosestZombie = nullptr;
	float ClosestDistance = FLT_MAX;

	for (const FKnownZombie& Zombie : KnownZombies)
	{
		if (!IsValid(Zombie.Actor))
		{
			continue;
		}

		const float Distance =
			FVector::Dist2D(OwnerLocation, Zombie.LastKnownLocation);

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestZombie = Zombie.Actor;
		}
	}

	return ClosestZombie;
}

bool UBTTask_ShootClosestZombie::IsFacingTarget(
	APawn* OwnerPawn,
	const FVector& TargetLocation) const
{
	if (!OwnerPawn)
	{
		return false;
	}

	FVector ToTarget = TargetLocation - OwnerPawn->GetActorLocation();
	ToTarget.Z = 0.0f;

	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	ToTarget.Normalize();

	FVector Forward = OwnerPawn->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	const float Dot = FVector::DotProduct(Forward, ToTarget);

	return Dot >= ShootFacingDotThreshold;
}

bool UBTTask_ShootClosestZombie::TryUseWeapon(APawn* OwnerPawn) const
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

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int Slot = 0; Slot < Inventory->GetInventoryCapacity(); ++Slot)
	{
		if (!Items.IsValidIndex(Slot) || !Items[Slot])
		{
			continue;
		}

		const bool bIsWeapon =
			Items[Slot]->GetItemType() == EItemType::Pistol ||
			Items[Slot]->GetItemType() == EItemType::Shotgun;

		if (!bIsWeapon)
		{
			continue;
		}

		if (Items[Slot]->GetValue() <= 0)
		{
			Inventory->RemoveItem(Slot);
			continue;
		}

		if (Inventory->UseItem(Slot))
		{
			if (Items.IsValidIndex(Slot) && Items[Slot] && Items[Slot]->GetValue() <= 0)
			{
				Inventory->RemoveItem(Slot);
			}

			return true;
		}
	}

	return false;
}