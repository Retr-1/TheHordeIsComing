#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;

UCLASS()
class PERLINNOISEGEN_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

protected:
    // ↓ This was missing
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* Health;

    UFUNCTION()
    void OnZeroHealth();

    UFUNCTION()
    void OnHealthChanged(float NewHealth, float Delta);
};
