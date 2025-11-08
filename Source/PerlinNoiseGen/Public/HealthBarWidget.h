#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class PERLINNOISEGEN_API UHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HealthBar")
    void SetHealthPercent(float Percent);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))        // must match the UMG widget name: "HealthBar"
        UProgressBar* HealthBar = nullptr;
};
