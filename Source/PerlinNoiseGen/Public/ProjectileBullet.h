#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class PERLINNOISEGEN_API AProjectileBullet : public AActor
{
    GENERATED_BODY()

public:
    AProjectileBullet();

protected:
    UPROPERTY(VisibleAnywhere)
    USphereComponent* Collision;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* Movement;

    UPROPERTY(EditAnywhere, Category = "Damage")
    float Damage = 20.f;

    virtual void BeginPlay() override;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);
};
