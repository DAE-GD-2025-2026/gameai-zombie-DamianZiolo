#pragma once

#include "CoreMinimal.h"
#include "AIWorldMemoryTypes.generated.h"

USTRUCT()
struct FKnownZombie
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY()
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY()
	float LastSeenTime = 0.0f;
};

USTRUCT()
struct FKnownItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY()
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bCollected = false;
};

USTRUCT()
struct FKnownHouse
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	bool bVisited = false;
};