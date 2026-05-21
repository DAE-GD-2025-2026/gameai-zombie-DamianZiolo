#include "StudentPerceptor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Village/House/House.h"
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"


UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	if (IsZombie(Actor))
	{
		HandleZombiePerception(Actor);
	}
	else if (IsHouse(Actor))
	{
		HandleHousePerception(Actor);
	}
	else if (IsItem(Actor))
	{
		HandleItemPerception(Actor);
	}
	
}

void UStudentPerceptor::HandleHousePerception(AActor* Actor)
{
	if (!Actor) return;
	
	for (FKnownHouse& KnownHouse : KnownHouses)
	{
		if (KnownHouse.Actor == Actor)
		{
			KnownHouse.Location = Actor->GetActorLocation();
			return;
		}
	}
	
	FKnownHouse NewHouse;
	NewHouse.Actor = Actor;
	NewHouse.Location = Actor->GetActorLocation();
	NewHouse.bVisited = false;
	
	KnownHouses.Add(NewHouse);
	
}

bool UStudentPerceptor::IsHouse(AActor* Actor) const
{
	return Actor && Actor->IsA(AHouse::StaticClass());
}

void UStudentPerceptor::MarkHouseVisited(AActor* Actor)
{
	if (!Actor) return;
	
	for (FKnownHouse& KnownHouse : KnownHouses)
	{
		if (KnownHouse.Actor == Actor)
		{
			KnownHouse.bVisited = true;
			return;
		}
	}
}

void UStudentPerceptor::HandleZombiePerception(AActor* Actor)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	
	for (FKnownZombie& KnownZombie : KnownZombies)
	{
		if (KnownZombie.Actor == Actor)
		{
			KnownZombie.LastKnownLocation = Actor->GetActorLocation();
			KnownZombie.LastSeenTime = CurrentTime;
			return;
		}
	}
	
	FKnownZombie NewZombie;
	NewZombie.Actor = Actor;
	NewZombie.LastKnownLocation = Actor->GetActorLocation();
	NewZombie.LastSeenTime = CurrentTime;
	
	KnownZombies.Add(NewZombie);
}

void UStudentPerceptor::CleanupExpiredZombies()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (int i = KnownZombies.Num() - 1; i >= 0; --i)
	{
		const bool bInvalidActor = !IsValid(KnownZombies[i].Actor);
		const bool bMemoryExpired = CurrentTime - KnownZombies[i].LastSeenTime > MemoryDuration;
		
		if (bInvalidActor || bMemoryExpired)
		{
			KnownZombies.RemoveAt(i);
		}
	}
}

void UStudentPerceptor::HandleItemPerception(AActor* Actor)
{
	if (!Actor) return;
	for (FKnownItem& KnownItem: KnownItems)
	{
		if (KnownItem.Actor == Actor)
		{
			KnownItem.LastKnownLocation = Actor->GetActorLocation();
			KnownItem.bCollected = false;
			return;
		}
	}
	
	FKnownItem NewItem;
	NewItem.Actor = Actor;
	NewItem.LastKnownLocation = Actor->GetActorLocation();
	NewItem.bCollected = false;
	KnownItems.Add(NewItem);
	
}

bool UStudentPerceptor::IsItem(AActor* Actor) const
{
	return Actor && Actor->IsA(ABaseItem::StaticClass());
}

void UStudentPerceptor::CleanupKnownItems()
{
	for (int i = KnownItems.Num() - 1; i >= 0; --i)
	{
		const bool bInvalidItem = !IsValid(KnownItems[i].Actor);
		const bool bHiddenItem = IsValid(KnownItems[i].Actor) && KnownItems[i].Actor->IsHidden();

		if (bInvalidItem || bHiddenItem)
		{
			KnownItems.RemoveAt(i);
		}
	}
}

void UStudentPerceptor::RemoveKnownItem(AActor* ItemActor)
{
	if (!ItemActor) return;
	
	for (int i = KnownItems.Num() - 1; i >= 0; --i)
	{
		if (KnownItems[i].Actor == ItemActor)
		{
			KnownItems.RemoveAt(i);
			return;
		}
	}
}

bool UStudentPerceptor::IsZombie(AActor* Actor) const
{
	return Actor && Actor->IsA(ABaseZombie::StaticClass());
}

APawn* UStudentPerceptor::GetControlledPawn() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	if (AAIController* AIController = Cast<AAIController>(Owner))
	{
		return AIController->GetPawn();
	}
	return Cast<APawn>(Owner);
}

const TArray<FKnownZombie>& UStudentPerceptor::GetKnownZombies() const
{
	return KnownZombies;
}

const TArray<FKnownItem>& UStudentPerceptor::GetKnownItems()
{
	CleanupKnownItems();
	return KnownItems;
}

const TArray<FKnownHouse>& UStudentPerceptor::GetKnownHouses() const
{
	return KnownHouses;
}
