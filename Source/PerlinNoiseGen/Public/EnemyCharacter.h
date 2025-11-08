#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UHealthComponent;
class UWidgetComponent;

UCLASS()
class PERLINNOISEGEN_API AEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UHealthComponent* Health;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* HealthBarWidgetComp;

    UFUNCTION()
    void OnZeroHealth();

    UFUNCTION()
    void OnHealthChanged(float NewHealth, float Delta);

    void UpdateHealthBar(); // convenience
};
