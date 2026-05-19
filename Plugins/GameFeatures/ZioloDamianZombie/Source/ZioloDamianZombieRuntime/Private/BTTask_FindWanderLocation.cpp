#include "BTTask_FindWanderLocation.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_FindWanderLocation::UBTTask_FindWanderLocation()
{
	NodeName = TEXT("Find Wander Location");
}

EBTNodeResult::Type UBTTask_FindWanderLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	const FVector AgentLocation = Pawn->GetActorLocation();

	const float YawRad =
		FMath::DegreesToRadians(Pawn->GetActorRotation().Yaw);

	const FVector Forward = FVector(
		FMath::Cos(YawRad),
		FMath::Sin(YawRad),
		0.0f
	);

	const FVector CircleCenter =
		AgentLocation + Forward * OffsetDistance;

	const float DeltaAngle =
		FMath::FRandRange(-MaxAngleChange, MaxAngleChange);

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

	FVector WanderTarget =
		CircleCenter + OffsetOnCircle * Radius;

	// Project onto NavMesh
	if (UNavigationSystemV1* NavSystem =
		UNavigationSystemV1::GetCurrent(Pawn->GetWorld()))
	{
		FNavLocation ProjectedLocation;

		if (NavSystem->ProjectPointToNavigation(
			WanderTarget,
			ProjectedLocation))
		{
			WanderTarget = ProjectedLocation.Location;
		}
	}

	OwnerComp.GetBlackboardComponent()->SetValueAsVector(
		TargetLocationKey.SelectedKeyName,
		WanderTarget
	);

	return EBTNodeResult::Succeeded;
}