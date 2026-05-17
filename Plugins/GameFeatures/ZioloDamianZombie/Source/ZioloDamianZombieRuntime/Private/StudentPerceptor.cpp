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

	if (!IsZombie(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		VisibleZombies.AddUnique(Actor);
	}
	else
	{
		VisibleZombies.Remove(Actor);
	}

	UpdateThreatBlackboard();
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

	for (int i = VisibleZombies.Num() - 1; i >= 0; --i)
	{
		if (!IsValid(VisibleZombies[i]))
		{
			VisibleZombies.RemoveAt(i);
		}
	}

	if (VisibleZombies.IsEmpty())
	{
		Blackboard->ClearValue(TEXT("TargetZombie"));
		return;
	}

	const FVector SurvivorLocation = Survivor->GetActorLocation();

	FVector FleeDirection = FVector::ZeroVector;
	AActor* ClosestZombie = nullptr;
	float ClosestDistance = FLT_MAX;

	for (AActor* Zombie : VisibleZombies)
	{
		const FVector ZombieLocation = Zombie->GetActorLocation();

		FVector Away = SurvivorLocation - ZombieLocation;
		Away.Z = 0.f;

		const float Distance = FMath::Max(Away.Size(), 1.f);
		Away.Normalize();

		FleeDirection += Away / Distance;

		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestZombie = Zombie;
		}
	}

	if (!ClosestZombie || FleeDirection.IsNearlyZero())
	{
		return;
	}

	FleeDirection.Normalize();

	const FVector FleeLocation = SurvivorLocation + FleeDirection * 1000.f;

	Blackboard->SetValueAsObject(TEXT("TargetZombie"), ClosestZombie);
	Blackboard->SetValueAsVector(TEXT("TargetLocation"), FleeLocation);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.0f,
			FColor::Red,
			FString::Printf(TEXT("Evading %d zombie(s)"), VisibleZombies.Num())
		);
	}
}


