#include "ProjectileBullet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AProjectileBullet::AProjectileBullet()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(6.f);
    Collision->SetCollisionProfileName(TEXT("Projectile")); // preset should Block Pawn/World
    Collision->SetNotifyRigidBodyCollision(true);
    RootComponent = Collision;

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->InitialSpeed = 4000.f;
    Movement->MaxSpeed = 4000.f;
    Movement->ProjectileGravityScale = 0.f; // bullets often have 0 or very low gravity

    // Bind hit
    Collision->OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);
}

void AProjectileBullet::BeginPlay()
{
    Super::BeginPlay();
    // Optional: lifespan
    SetLifeSpan(5.f);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this)
    {
        // Apply point damage (respects OnTakePointDamage on the target)
        const FVector ShotDir = GetVelocity().GetSafeNormal();
        UGameplayStatics::ApplyPointDamage(
            OtherActor, Damage, ShotDir, Hit,
            GetInstigatorController(), this, UDamageType::StaticClass()
        );
    }

    Destroy(); // bullet consumed on impact
}
