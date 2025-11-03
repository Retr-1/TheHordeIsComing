#include "EnemyCharacter.h"
#include "WanderAIController.h"
#include "GameFramework/CharacterMovementComponent.h"   // <-- add this

AEnemyCharacter::AEnemyCharacter()
{
    AIControllerClass = AWanderAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    Tags.Add(FName("Enemy"));

    if (auto* Move = GetCharacterMovement()) {
        Move->bUseRVOAvoidance = true;
        Move->MaxWalkSpeed = 300.f;
    }
}