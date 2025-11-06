#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthZero);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PERLINNOISEGEN_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHealthComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    float CurrentHealth = 100.f;

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthZero OnHealthZero;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Heal(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Damage(float Amount);

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleAnyDamage(AActor* DamagedActor, float Damage,
        const class UDamageType* DamageType,
        class AController* InstigatedBy, AActor* DamageCauser);

    UFUNCTION()
    void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy,
        FVector HitLocation, class UPrimitiveComponent* FHitComponent,
        FName BoneName, FVector ShotFromDirection,
        const class UDamageType* DamageType, AActor* DamageCauser);

    void ApplyDamageInternal(float Amount);
};
