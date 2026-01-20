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

UImage* UMapWidget::EnsureActorIcon(AActor* Actor, UTexture2D* IconTex, const FVector2D& IconSize)
{
    if (!RootCanvas || !Actor || !IconTex) return nullptr;

    if (UImage* Existing = ActorToIconImage.FindRef(Actor))
    {
        Existing->SetBrushFromTexture(IconTex, true);

        if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(Existing->Slot))
        {
            CSlot->SetSize(IconSize);
            CSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        }
        return Existing;
    }

    UImage* NewImg = NewObject<UImage>(this);
    if (!NewImg) return nullptr;

    NewImg->SetBrushFromTexture(IconTex, true);
    NewImg->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

    // Add to canvas
    UCanvasPanelSlot* CSlot = RootCanvas->AddChildToCanvas(NewImg);
    if (CSlot)
    {
        CSlot->SetSize(IconSize);
        CSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CSlot->SetZOrder(10); // above map image, below arrow if needed
    }

    ActorToIconImage.Add(Actor, NewImg);
    return NewImg;
}

void UMapWidget::RemoveActorIcon(AActor* Actor)
{
    if (!Actor) return;

    if (TObjectPtr<UImage>* Found = ActorToIconImage.Find(Actor))
    {
        if (UImage* Img = Found->Get())
        {
            Img->RemoveFromParent();
        }
        ActorToIconImage.Remove(Actor);
    }
}

void UMapWidget::ClearAllActorIcons()
{
    for (auto& KVP : ActorToIconImage)
    {
        if (UImage* Img = KVP.Value)
        {
            Img->RemoveFromParent();
        }
    }
    ActorToIconImage.Empty();
}

void UMapWidget::SetActorIconPosition(AActor* Actor, float U, float V)
{
    if (!Actor) return;

    UImage* Img = ActorToIconImage.FindRef(Actor);
    if (!Img) return;

    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Img->Slot);
    if (!CanvasSlot || !RootCanvas) return;

    const FVector2D DisplaySize = GetDisplayedMapSize();
    const FVector2D CanvasSize = RootCanvas->GetCachedGeometry().GetLocalSize();

    const float X = (U - 0.5f) * DisplaySize.X + CanvasSize.X * 0.5f;
    const float Y = (V - 0.5f) * DisplaySize.Y + CanvasSize.Y * 0.5f;

    CanvasSlot->SetPosition(FVector2D(X, Y));
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