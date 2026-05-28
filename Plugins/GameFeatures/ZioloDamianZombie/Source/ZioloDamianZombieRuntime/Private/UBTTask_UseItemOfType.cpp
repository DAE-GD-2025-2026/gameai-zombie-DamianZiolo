#include "UBTTask_UseItemOfType.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

UBTTask_UseItemOfType::UBTTask_UseItemOfType()
{
	NodeName = TEXT("Use Item Of Type");
}

EBTNodeResult::Type UBTTask_UseItemOfType::ExecuteTask(
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

	UInventoryComponent* Inventory =
		OwnerPawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return EBTNodeResult::Failed;
	}

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int Slot = 0; Slot < Inventory->GetInventoryCapacity(); ++Slot)
	{
		if (!Items.IsValidIndex(Slot) || !Items[Slot])
		{
			continue;
		}

		if (Items[Slot]->GetItemType() != ItemType)
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

			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}