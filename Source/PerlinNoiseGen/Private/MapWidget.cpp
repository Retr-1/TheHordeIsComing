#include "MapWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

void UMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Make sure the arrow rotates around its center
    if (PlayerArrow)
    {
        PlayerArrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    }
}

void UMapWidget::SetMapTexture(UTexture2D* Texture)
{
    if (!MapImage || !Texture) return;

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    MapImage->SetBrush(Brush);

    MapPixelSize = Brush.ImageSize;

    // Optional: ensure the map image slot matches the texture size (if it’s in a Canvas)
    if (UCanvasPanelSlot* MapCanvasSlot = Cast<UCanvasPanelSlot>(MapImage->Slot))
    {
        MapCanvasSlot->SetSize(MapPixelSize);
    }
}

void UMapWidget::UpdatePlayerArrowTransform(float YawDegrees, float U, float V)
{
    if (!PlayerArrow) return;

    // Rotate arrow (yaw)
    PlayerArrow->SetRenderTransformAngle(YawDegrees + ArrowYawOffset);

    // Place arrow using Canvas slot position so anchors/layout are respected
    if (UCanvasPanelSlot* ArrowSlot = Cast<UCanvasPanelSlot>(PlayerArrow->Slot))
    {
        // IMPORTANT: If your texture rows are "top = +Y", you probably want V flipped here.
        // If you already flipped Y during texture generation, do NOT flip again.
        // Start with no flip; if it's vertically mirrored, set V = 1.f - V.
        const float X = (1.f - U) * MapPixelSize.X;   // flip U
        const float Y = (1.f - V) * MapPixelSize.Y;

        ArrowSlot->SetAlignment(FVector2D(0.5f, 0.5f));   // arrow centered on point
        ArrowSlot->SetPosition(FVector2D(X, Y));
    }
}
