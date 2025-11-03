#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "WanderAIController.generated.h"

UCLASS()
class PERLINNOISEGEN_API AWanderAIController : public AAIController
{
    GENERATED_BODY()
public:
    AWanderAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

    // Must be parameterless to bind directly to SetTimer
    void PickNextDestination();

    void ScheduleNextDestination(float Delay);

    UPROPERTY(EditAnywhere, Category = "Wander") float WanderRadius = 1200.f;
    UPROPERTY(EditAnywhere, Category = "Wander") float AcceptanceRadius = 90.f;
    UPROPERTY(EditAnywhere, Category = "Wander") FVector2D WaitRange = FVector2D(0.5f, 1.5f);
    UPROPERTY(EditAnywhere, Category = "Wander") float DefaultWalkSpeed = 300.f;

    FTimerHandle WaitTimer;
};

