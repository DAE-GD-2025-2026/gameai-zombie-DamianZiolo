#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Items/ItemType.h"
#include "UBTTask_UseItemOfTypeZioloDamian.generated.h"

UCLASS()
class ZIOLODAMIANZOMBIERUNTIME_API UBTTask_UseItemOfTypeZioloDamian : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UseItemOfTypeZioloDamian();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

public:
	UPROPERTY(EditAnywhere, Category = "Item")
	EItemType ItemType = EItemType::Food;
};