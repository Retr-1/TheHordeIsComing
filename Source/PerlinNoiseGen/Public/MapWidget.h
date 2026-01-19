#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapWidget.generated.h"

class UImage;

UCLASS()
class PERLINNOISEGEN_API UMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetMapTexture(UTexture2D* Texture);

    UFUNCTION(BlueprintCallable, Category = "Map")
    void UpdatePlayerArrowTransform(float YawDegrees, float U, float V);

    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetArrowYawOffset(float InOffset) { ArrowYawOffset = InOffset; }

protected:
    virtual void NativeConstruct() override;

public:
    UPROPERTY(meta = (BindWidget))
    UImage* MapImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    UImage* PlayerArrow = nullptr;

private:
    FVector2D MapPixelSize = FVector2D(512.f, 512.f);
    float ArrowYawOffset = 0.f;
};
