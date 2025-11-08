#include "EnemyCharacter.h"
#include "WanderAIController.h"
#include "HealthComponent.h"
#include "HealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"

AEnemyCharacter::AEnemyCharacter()
{
    AIControllerClass = AWanderAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

    // --- Health bar widget component sitting above the head ---
    HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
    HealthBarWidgetComp->SetupAttachment(GetCapsuleComponent());
    HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);   // faces the screen automatically
    HealthBarWidgetComp->SetDrawAtDesiredSize(true);
    HealthBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 110.f)); // tweak height to your mesh
    HealthBarWidgetComp->SetTickWhenOffscreen(false);             // perf
    // WidgetClass will be assigned in the Editor to WB_HealthBar

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

    UpdateHealthBar(); // initialize to current health
}

void AEnemyCharacter::UpdateHealthBar()
{
    if (!HealthBarWidgetComp) return;

    if (UUserWidget* UW = HealthBarWidgetComp->GetUserWidgetObject())
    {
        if (UHealthBarWidget* HB = Cast<UHealthBarWidget>(UW))
        {
            const float P = (Health && Health->GetMaxHealth() > 0.f)
                ? Health->GetCurrentHealth() / Health->GetMaxHealth()
                : 0.f;
            HB->SetHealthPercent(P);
        }
    }
}

void AEnemyCharacter::OnHealthChanged(float NewHealth, float Delta)
{
    UpdateHealthBar();
}

void AEnemyCharacter::OnZeroHealth()
{
    if (auto* Move = GetCharacterMovement())
    {
        Move->DisableMovement();
    }
    // Hide the bar right away (optional)
    if (HealthBarWidgetComp) HealthBarWidgetComp->SetVisibility(false);
    Destroy();
}

void AEnemyCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HealthBarWidgetComp) return;
    if (HealthBarWidgetComp->GetWidgetSpace() != EWidgetSpace::World) return;

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
        const FVector WidgetLoc = HealthBarWidgetComp->GetComponentLocation();
        FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(WidgetLoc, CamLoc);

        LookAt.Pitch = 0.f; LookAt.Roll = 0.f; // keep it horizontal
        HealthBarWidgetComp->SetWorldRotation(LookAt);
    }
}