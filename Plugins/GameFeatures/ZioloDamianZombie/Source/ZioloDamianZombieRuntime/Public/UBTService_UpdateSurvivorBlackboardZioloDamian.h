#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "Items/ItemType.h"
#include "AIWorldMemoryTypesZioloDamian.h"
#include "UBTService_UpdateSurvivorBlackboardZioloDamian.generated.h"
UCLASS()
class ZIOLODAMIANZOMBIERUNTIME_API UBTService_UpdateSurvivorBlackboardZioloDamian : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateSurvivorBlackboardZioloDamian();

protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;

private:
	bool HasWeapon(APawn* OwnerPawn) const;
	bool HasItemOfType(APawn* OwnerPawn, EItemType ItemType) const;
	bool HasNearbyThreat(const TArray<FKnownZombie>& KnownZombies, const FVector& OwnerLocation) const;
	bool HasKnownUnvisitedHouse(const TArray<FKnownHouse>& KnownHouses) const;
};