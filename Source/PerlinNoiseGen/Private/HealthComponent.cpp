#include "HealthComponent.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = FMath::Max(1.f, MaxHealth);

    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleAnyDamage);
    }
}

void UHealthComponent::Heal(float Amount)
{
    if (Amount <= 0.f || CurrentHealth <= 0.f) return;
    const float Old = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - Old);
}

void UHealthComponent::Damage(float Amount)
{
    if (Amount <= 0.f || CurrentHealth <= 0.f) return;
    ApplyDamageInternal(Amount);
}

void UHealthComponent::HandleAnyDamage(AActor*, float Damage, const UDamageType*, AController*, AActor*)
{
    if (Damage > 0.f) ApplyDamageInternal(Damage);
}

void UHealthComponent::ApplyDamageInternal(float Amount)
{
    if (CurrentHealth <= 0.f) return;

    const float Old = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - Old);

    if (CurrentHealth <= 0.f)
    {
        OnHealthZero.Broadcast();
    }
}
