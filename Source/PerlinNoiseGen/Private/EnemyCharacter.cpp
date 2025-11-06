#include "EnemyCharacter.h"
#include "WanderAIController.h"
#include "HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    AIControllerClass = AWanderAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

    if (auto* Move = GetCharacterMovement())
    {
        Move->bUseRVOAvoidance = true;
        Move->MaxWalkSpeed = 300.f;
    }

    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetNotifyRigidBodyCollision(false);
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (Health)
    {
        Health->OnHealthZero.AddDynamic(this, &AEnemyCharacter::OnZeroHealth);
        Health->OnHealthChanged.AddDynamic(this, &AEnemyCharacter::OnHealthChanged);
    }
}

void AEnemyCharacter::OnZeroHealth()
{
    if (auto* Move = GetCharacterMovement())
    {
        Move->DisableMovement();
    }
    Destroy();
}

void AEnemyCharacter::OnHealthChanged(float NewHealth, float Delta)
{
    // Optional: hit-react, material flash, debug log, etc.
    // UE_LOG(LogTemp, Log, TEXT("Enemy health: %.1f (Δ%.1f)"), NewHealth, Delta);
}
