#include "HealthBarWidget.h"
#include "Components/ProgressBar.h"

void UHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (HealthBar)
    {
        HealthBar->SetPercent(1.0f); // start full by default
    }
}

void UHealthBarWidget::SetHealthPercent(float Percent)
{
    if (HealthBar)
    {
        UE_LOG(LogTemp, Warning, TEXT("Setting Perc %f"), FMath::Clamp(Percent, 0.f, 1.f));
        HealthBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
        
    }
}
