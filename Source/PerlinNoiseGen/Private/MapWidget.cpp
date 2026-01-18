#include "MapWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UMapWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMapWidget::SetMapTexture(UTexture2D* Texture)
{
    if (!MapImage || !Texture) return;

    MapPixelSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = MapPixelSize;
    MapImage->SetBrush(Brush);
}

void UMapWidget::UpdatePlayerArrow(float PawnYawDegrees)
{
    if (!PlayerArrow) return;
    PlayerArrow->SetRenderTransformAngle(PawnYawDegrees + ArrowYawOffset);
}

void UMapWidget::UpdatePlayerArrowTransform(float PawnYawDegrees, float U, float V)
{
    if (!PlayerArrow) return;

    PlayerArrow->SetRenderTransformAngle(PawnYawDegrees + ArrowYawOffset);

    const float X = U * MapPixelSize.X;
    const float Y = V * MapPixelSize.Y;

    FWidgetTransform T = PlayerArrow->GetRenderTransform();
    T.Translation = FVector2D(X, Y);
    PlayerArrow->SetRenderTransform(T);
}
