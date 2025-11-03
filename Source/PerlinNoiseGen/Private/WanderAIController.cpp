#include "WanderAIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AWanderAIController::AWanderAIController()
{
    bAttachToPawn = true;
}

void AWanderAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ACharacter* C = Cast<ACharacter>(InPawn))
        if (auto* M = C->GetCharacterMovement()) M->MaxWalkSpeed = DefaultWalkSpeed;

    // immediate first pick
    PickNextDestination();
}

void AWanderAIController::OnMoveCompleted(FAIRequestID, const FPathFollowingResult&)
{
    const float Wait = FMath::FRandRange(WaitRange.X, WaitRange.Y);
    ScheduleNextDestination(Wait);
}

void AWanderAIController::ScheduleNextDestination(float Delay)
{
    GetWorldTimerManager().SetTimer(WaitTimer, this, &AWanderAIController::PickNextDestination, Delay, false);
}

void AWanderAIController::PickNextDestination()
{
    if (!GetPawn()) { ScheduleNextDestination(0.5f); return; }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) { ScheduleNextDestination(0.5f); return; }

    const FVector Origin = GetPawn()->GetActorLocation();
    FNavLocation Pt;
    bool bFound = false;

    for (int32 i = 0; i < 8; ++i)
        if (NavSys->GetRandomReachablePointInRadius(Origin, WanderRadius, Pt)) { bFound = true; break; }

    if (bFound)
    {
        MoveToLocation(Pt.Location, AcceptanceRadius, /*bStopOnOverlap*/true, /*bUsePathfinding*/true, /*bProjectDestination*/true, /*bCanStrafe*/false);
    }
    else
    {
        // retry shortly near edges/obstacles
        ScheduleNextDestination(0.4f);
    }
}
