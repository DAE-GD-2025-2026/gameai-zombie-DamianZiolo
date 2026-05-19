// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptor.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Zombies/BaseZombie.h"

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
	//else
	//{
	//	VisibleZombies.Remove(Actor);
	//}

	UpdateThreatBlackboard();
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
	
	//If this is a new zombie, I'm adding it to the array
	FKnownZombie NewZombie;
	NewZombie.Actor = Actor;
	NewZombie.LastKnownLocation = Actor->GetActorLocation();
	NewZombie.LastSeenTime = CurrentTime;
	
	KnownZombies.Add(NewZombie);
	
}

//if ZOmbie is invalid ex. dead or MemoryExpired remove it from memory
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

void UStudentPerceptor::UpdateThreatBlackboard()
{
	APawn* Survivor = GetControlledPawn();
	if (!Survivor) return;

	AAIController* AIController = Cast<AAIController>(Survivor->GetController());
	if (!AIController) return;

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard) return;
	
	CleanupExpiredZombies();

	if (KnownZombies.IsEmpty())
	{
		Blackboard->ClearValue(TEXT("TargetZombie"));
		return;
	}

	const FVector SurvivorLocation = Survivor->GetActorLocation();

	const float ThreatRange = 1500.0f; // 15 meters
	const float FleeDistance = 1000.0f;

	FVector FleeDirection = FVector::ZeroVector;
	AActor* ClosestZombie = nullptr;
	float ClosestDistance = FLT_MAX;
	int ThreatCount = 0;

	for (const FKnownZombie& KnownZombie : KnownZombies)
	{
		const FVector ZombieLocation = KnownZombie.LastKnownLocation;

		FVector Away = SurvivorLocation - ZombieLocation;
		Away.Z = 0.f;

		const float Distance = FMath::Max(Away.Size(), 1.f);

		if (Distance > ThreatRange)
		{
			continue;
		}

		Away.Normalize();

		FleeDirection += Away / Distance;
		ThreatCount++;

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestZombie = KnownZombie.Actor;
		}
	}

	if (!ClosestZombie || FleeDirection.IsNearlyZero())
	{
		Blackboard->ClearValue(TEXT("TargetZombie"));
		return;
	}

	FleeDirection.Normalize();

	const FVector FleeLocation = SurvivorLocation + FleeDirection * FleeDistance;

	Blackboard->SetValueAsObject(TEXT("TargetZombie"), ClosestZombie);
	Blackboard->SetValueAsVector(TEXT("TargetLocation"), FleeLocation);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.0f,
			FColor::Red,
			FString::Printf(TEXT("Evading %d nearby zombie(s)"), ThreatCount)
		);
	}
}


