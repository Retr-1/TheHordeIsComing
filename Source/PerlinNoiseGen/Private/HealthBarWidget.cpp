#include "HealthBarWidget.h"
#include "Components/ProgressBar.h"

void UHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (HealthBar)
    {
        HealthBar->SetPercent(1.f); // start full by default
    }
}

void UHealthBarWidget::SetHealthPercent(float Percent)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(FMath::Clamp(Percent, 0.f, 1.f));
    }
}
