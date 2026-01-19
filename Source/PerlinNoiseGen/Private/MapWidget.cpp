// ===============================
// MapWidget.cpp
// ===============================
#include "MapWidget.h"

#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"

void UMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Rotate around center so the arrow spins properly
    if (PlayerArrow)
    {
        PlayerArrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    }
}

void UMapWidget::SetArrowYawOffset(float InOffsetDegrees)
{
    ArrowYawOffset = InOffsetDegrees;
}

void UMapWidget::SetMapTexture(UTexture2D* Texture)
{
    if (!MapImage || !Texture) return;

    // Reliable way to update UMG image brush resource
    MapImage->SetBrushFromTexture(Texture, /*bMatchSize=*/true);
    UE_LOG(LogTemp, Warning, TEXT("Rebuild IN"));

    MapPixelSize = FVector2D((float)Texture->GetSizeX(), (float)Texture->GetSizeY());

    // If MapImage sits in a CanvasPanel, make sure its slot matches the texture size
    if (UCanvasPanelSlot* MapCanvasSlot = Cast<UCanvasPanelSlot>(MapImage->Slot))
    {
        MapCanvasSlot->SetSize(MapPixelSize);
    }

    // Force Slate to rebuild/render using the new resource
    MapImage->InvalidateLayoutAndVolatility();
    MapImage->SynchronizeProperties();
    InvalidateLayoutAndVolatility();
}

void UMapWidget::UpdatePlayerArrowTransform(float YawDegrees, float U, float V)
{
    if (!PlayerArrow) return;

    // Rotate arrow
    PlayerArrow->SetRenderTransformAngle(YawDegrees + ArrowYawOffset);

    // Place arrow: this assumes U,V already match your texture orientation.
    // (If you flipped V in WorldToMapUV, do NOT flip here again.)
    if (UCanvasPanelSlot* ArrowCanvasSlot = Cast<UCanvasPanelSlot>(PlayerArrow->Slot))
    {
        const float X = U * MapPixelSize.X;
        const float Y = V * MapPixelSize.Y;

        ArrowCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // center on point
        ArrowCanvasSlot->SetPosition(FVector2D(X, Y));
    }
}
