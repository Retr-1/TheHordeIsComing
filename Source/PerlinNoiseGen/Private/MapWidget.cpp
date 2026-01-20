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
        const FVector2D DisplaySize = GetDisplayedMapSize();
        const FVector2D CanvasSize = RootCanvas->GetCachedGeometry().GetLocalSize();
        const float X = (U-0.5f) * DisplaySize.X + CanvasSize.X/2;
        const float Y = (V-0.5f) * DisplaySize.Y + CanvasSize.Y/2;
        UE_LOG(LogTemp, Warning, TEXT("X %f, Y %f"), X, Y);

        ArrowCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // center on point
        ArrowCanvasSlot->SetPosition(FVector2D(X, Y));
    }
}


FVector2D UMapWidget::GetDisplayedMapSize() const
{
    // Best: actual geometry size of MapImage at runtime
    if (MapScaleBox)
    {
        const FGeometry& Geo = MapScaleBox->GetCachedGeometry();
        const FVector2D Size = Geo.GetLocalSize();

        // CachedGeometry can be (0,0) on first frame; fall back
        if (Size.X > 1.f && Size.Y > 1.f)
        {
            return Size;
        }
    }

    // Fallback to texture size if geometry not ready
    return MapPixelSize;
}