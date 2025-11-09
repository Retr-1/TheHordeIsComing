#include "ProjectileBullet.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

AProjectileBullet::AProjectileBullet()
{
    PrimaryActorTick.bCanEverTick = false;

    // --- Collision ---
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(6.f);
    SetRootComponent(Collision);

    // Query-only so it won’t impart physics push; still generates hits
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    Collision->SetNotifyRigidBodyCollision(true);
    Collision->SetGenerateOverlapEvents(false);

    Collision->OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);

    // --- Movement ---
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->InitialSpeed = 4000.f;
    Movement->MaxSpeed = 4000.f;
    Movement->ProjectileGravityScale = 0.f;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    Movement->OnProjectileStop.AddDynamic(this, &AProjectileBullet::OnProjectileStop);

    // Safety timeout
    SetLifeSpan(5.f);
}

void AProjectileBullet::BeginPlay()
{
    Super::BeginPlay();

    // Ensure Instigator is set even if spawned from BP with only Owner filled
    if (!GetInstigator() && GetOwner())
    {
        if (APawn* AsPawn = Cast<APawn>(GetOwner()))
        {
            SetInstigator(AsPawn);
        }
    }
}

void AProjectileBullet::OnHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/,
    const FHitResult& Hit)
{
    ApplyDamageAndDie(Hit, OtherActor);
}

void AProjectileBullet::OnProjectileStop(const FHitResult& ImpactResult)
{
    // Fallback if movement stops without hit/overlap firing
    ApplyDamageAndDie(ImpactResult, ImpactResult.GetActor());
}

void AProjectileBullet::ApplyDamageAndDie(const FHitResult& Hit, AActor* OtherActor)
{
    if (OtherActor && OtherActor != this)
    {
        const FVector ShotDir = GetVelocity().GetSafeNormal();
        AController* InstigatorCtrl = GetInstigatorController();

        UE_LOG(LogTemp, Warning, TEXT("CALLING HITTTT!!!!!"));

        UGameplayStatics::ApplyPointDamage(
            OtherActor,
            Damage,
            ShotDir,
            Hit,
            InstigatorCtrl,
            this,
            DamageType ? *DamageType : UDamageType::StaticClass()
        );
    }

    Destroy();
}
