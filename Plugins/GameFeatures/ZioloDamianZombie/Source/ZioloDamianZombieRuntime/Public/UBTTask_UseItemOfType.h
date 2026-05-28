#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Items/ItemType.h"
#include "UBTTask_UseItemOfType.generated.h"

UCLASS()
class ZIOLODAMIANZOMBIERUNTIME_API UBTTask_UseItemOfType : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UseItemOfType();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

public:
	UPROPERTY(EditAnywhere, Category = "Item")
	EItemType ItemType = EItemType::Food;
};