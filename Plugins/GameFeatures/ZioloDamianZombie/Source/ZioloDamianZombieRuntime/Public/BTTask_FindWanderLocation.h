#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BTTask_FindWanderLocation.generated.h"

UCLASS()
class ZIOLODAMIANZOMBIERUNTIME_API UBTTask_FindWanderLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindWanderLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float OffsetDistance = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float Radius = 400.0f;
	
	UPROPERTY(EditAnywhere, Category = "Wander")
	float MaxAngleChange = 0.5f;

	float WanderAngle = 0.0f;
};