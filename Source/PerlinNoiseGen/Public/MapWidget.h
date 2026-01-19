// ===============================
// MapWidget.h
// ===============================
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class PERLINNOISEGEN_API UMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Sets the map texture on the UMG Image and forces a UI refresh.
    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetMapTexture(UTexture2D* Texture);

    // Updates arrow rotation and position from normalized UV in [0..1]
    UFUNCTION(BlueprintCallable, Category = "Map")
    void UpdatePlayerArrowTransform(float YawDegrees, float U, float V);

    // Allows aligning arrow art (e.g., if the arrow texture points right by default, use -90 to make it point up)
    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetArrowYawOffset(float InOffsetDegrees);

protected:
    virtual void NativeConstruct() override;

public:
    // Bind these in your Widget Blueprint (same names), or assign manually in BP
    UPROPERTY(meta = (BindWidget))
    UImage* MapImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    UImage* PlayerArrow = nullptr;

private:
    // Cached map size in pixels (from the current texture)
    FVector2D MapPixelSize = FVector2D(512.f, 512.f);

    // Degrees added to the pawn/controller yaw so arrow art matches world forward
    float ArrowYawOffset = 0.f;
};
