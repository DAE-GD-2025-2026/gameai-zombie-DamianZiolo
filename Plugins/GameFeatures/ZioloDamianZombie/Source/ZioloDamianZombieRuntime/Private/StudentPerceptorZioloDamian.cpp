#include "StudentPerceptorZioloDamian.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "Village/House/House.h"
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"


UStudentPerceptorZioloDamian::UStudentPerceptorZioloDamian()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptorZioloDamian::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorZioloDamian::OnPerceptionUpdated);
	}
}

void UStudentPerceptorZioloDamian::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.Type == UAISense::GetSenseID(UAISense_Damage::StaticClass()) )
	{
		HandleDamagePerception(Stimulus);
        return;
	}
	

	
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

void UStudentPerceptorZioloDamian::HandleHousePerception(AActor* Actor)
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

bool UStudentPerceptorZioloDamian::IsHouse(AActor* Actor) const
{
	return Actor && Actor->IsA(AHouse::StaticClass());
}

void UStudentPerceptorZioloDamian::MarkHouseVisited(AActor* Actor)
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

void UStudentPerceptorZioloDamian::HandleZombiePerception(AActor* Actor)
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

void UStudentPerceptorZioloDamian::CleanupExpiredZombies()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	for (int i = KnownZombies.Num() - 1; i >= 0; --i)
	{
		AActor* ZombieActor = KnownZombies[i].Actor;

		const bool bIsGhostZombie = ZombieActor == nullptr;
		const bool bInvalidActor = !bIsGhostZombie && !IsValid(ZombieActor);
		const bool bHiddenActor = !bIsGhostZombie && ZombieActor->IsHidden();
		const bool bMemoryExpired = CurrentTime - KnownZombies[i].LastSeenTime > MemoryDuration;

		if (bInvalidActor || bHiddenActor || bMemoryExpired)
		{
			KnownZombies.RemoveAt(i);
		}
	}
}

void UStudentPerceptorZioloDamian::HandleItemPerception(AActor* Actor)
{
	if (!Actor) return;
	if (Actor->GetClass()->GetName().Contains(TEXT("BP_Garbage")))
	{
		return;
	}
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

bool UStudentPerceptorZioloDamian::IsItem(AActor* Actor) const
{
	return Actor && Actor->IsA(ABaseItem::StaticClass());
}

void UStudentPerceptorZioloDamian::CleanupKnownItems()
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

void UStudentPerceptorZioloDamian::HandleDamagePerception(const FAIStimulus& Stimulus)
{
	APawn* Survivor = GetControlledPawn();
	if (!Survivor)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	const FVector SurvivorLocation = Survivor->GetActorLocation();
	const FVector BehindDirection = -Survivor->GetActorForwardVector();

	FVector GhostLocation = SurvivorLocation + BehindDirection * 300.0f;
	GhostLocation.Z = SurvivorLocation.Z;

	FKnownZombie GhostZombie;
	GhostZombie.Actor = nullptr;
	GhostZombie.LastKnownLocation = GhostLocation;
	GhostZombie.LastSeenTime = CurrentTime;

	KnownZombies.Add(GhostZombie);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.0f,
			FColor::Purple,
			TEXT("Damage stimulus: ghost zombie added behind survivor")
		);
	}
}

void UStudentPerceptorZioloDamian::RemoveKnownItem(AActor* ItemActor)
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

bool UStudentPerceptorZioloDamian::IsZombie(AActor* Actor) const
{
	return Actor && Actor->IsA(ABaseZombie::StaticClass());
}

APawn* UStudentPerceptorZioloDamian::GetControlledPawn() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	if (AAIController* AIController = Cast<AAIController>(Owner))
	{
		return AIController->GetPawn();
	}
	return Cast<APawn>(Owner);
}

const TArray<FKnownZombie>& UStudentPerceptorZioloDamian::GetKnownZombies()
{
	CleanupExpiredZombies();
	return KnownZombies;
}

const TArray<FKnownItem>& UStudentPerceptorZioloDamian::GetKnownItems()
{
	CleanupKnownItems();
	return KnownItems;
}

const TArray<FKnownHouse>& UStudentPerceptorZioloDamian::GetKnownHouses() const
{
	return KnownHouses;
}
