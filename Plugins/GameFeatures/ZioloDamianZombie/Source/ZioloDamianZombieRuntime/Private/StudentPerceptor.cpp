#include "StudentPerceptor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
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

void UStudentPerceptor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateThreatBlackboard();
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	if (IsZombie(Actor))
	{
		HandleZombiePerception(Actor);
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

	const float ThreatRange = 1500.0f;
	const float FleeDistance = 500.0f;

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

		//a little bit of priority to "more" evade closer enemies
		FleeDirection += Away / Distance;
		ThreatCount++;

		if (Distance < ClosestDistance && IsValid(KnownZombie.Actor))
		{
			ClosestDistance = Distance;
			ClosestZombie = KnownZombie.Actor;
		}
	}

	if (FleeDirection.IsNearlyZero())
	{
		Blackboard->ClearValue(TEXT("TargetZombie"));
		return;
	}

	FleeDirection.Normalize();

	FVector FleeLocation = SurvivorLocation + FleeDirection * FleeDistance;

	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;

		if (NavSystem->ProjectPointToNavigation(FleeLocation, ProjectedLocation))
		{
			FleeLocation = ProjectedLocation.Location;
		}
	}

	DrawThreatDebug(SurvivorLocation, FleeDirection, FleeLocation);

	Blackboard->SetValueAsObject(TEXT("TargetZombie"), ClosestZombie);
	Blackboard->SetValueAsVector(TEXT("TargetLocation"), FleeLocation);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			1,
			0.0f,
			FColor::Red,
			FString::Printf(TEXT("Evading %d nearby zombie(s) | Known: %d"), ThreatCount, KnownZombies.Num())
		);
	}
}

void UStudentPerceptor::DrawThreatDebug(
	const FVector& SurvivorLocation,
	const FVector& FleeDirection,
	const FVector& FleeLocation) const
{
	if (!GetWorld()) return;

	DrawDebugSphere(
		GetWorld(),
		FleeLocation,
		50.0f,
		16,
		FColor::Green,
		false,
		0.05f,
		0,
		3.0f
	);

	DrawDebugLine(
		GetWorld(),
		SurvivorLocation,
		SurvivorLocation + FleeDirection * 500.0f,
		FColor::Blue,
		false,
		0.05f,
		0,
		5.0f
	);

	for (const FKnownZombie& KnownZombie : KnownZombies)
	{
		DrawDebugSphere(
			GetWorld(),
			KnownZombie.LastKnownLocation,
			40.0f,
			12,
			FColor::Red,
			false,
			0.05f,
			0,
			2.5f
		);
	}
}