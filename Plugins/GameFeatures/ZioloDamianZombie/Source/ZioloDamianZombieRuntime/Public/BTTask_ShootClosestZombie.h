#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ShootClosestZombie.generated.h"

struct FKnownZombie;

UCLASS()
class ZIOLODAMIANZOMBIERUNTIME_API UBTTask_ShootClosestZombie : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ShootClosestZombie();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

private:
	AActor* GetClosestZombie(
		const TArray<FKnownZombie>& KnownZombies,
		const FVector& OwnerLocation
	) const;

	bool IsFacingTarget(
		APawn* OwnerPawn,
		const FVector& TargetLocation
	) const;

	bool TryUseWeapon(APawn* OwnerPawn) const;

private:
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootingRange = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShootFacingDotThreshold = 0.85f;
};