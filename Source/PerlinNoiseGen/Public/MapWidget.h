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
    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetMapTexture(UTexture2D* Texture);

    UFUNCTION(BlueprintCallable, Category = "Map")
    void SetArrowYawOffset(float Degrees) { ArrowYawOffset = Degrees; }

    UFUNCTION(BlueprintCallable, Category = "Map")
    void UpdatePlayerArrow(float PawnYawDegrees);

    UFUNCTION(BlueprintCallable, Category = "Map")
    void UpdatePlayerArrowTransform(float PawnYawDegrees, float U, float V);

protected:
    virtual void NativeConstruct() override;

public:
    UPROPERTY(meta = (BindWidget))
    UImage* MapImage = nullptr;

    UPROPERTY(meta = (BindWidget))
    UImage* PlayerArrow = nullptr;

private:
    FVector2D MapPixelSize = FVector2D(512, 512);
    float ArrowYawOffset = 0.f;
};
