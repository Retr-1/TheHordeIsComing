// ===============================
// MapTextureGenerator.h
// ===============================
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MapTextureGenerator.generated.h"

class UTexture2D;
class ANoiseTerrainActor;

USTRUCT(BlueprintType)
struct FMapGenSettings
{
    GENERATED_BODY()

    // Final texture resolution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution")
    int32 MapWidth = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution")
    int32 MapHeight = 512;

    // Marching squares resolution (cells). Field is (CellsX+1)x(CellsY+1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution")
    int32 CellsX = 256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution")
    int32 CellsY = 256;

    // Contour line thickness in pixels
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Contours", meta = (ClampMin = "1", UIMin = "1"))
    int32 LineThickness = 2;

    // Second height threshold: snow/peaks start at this height (Z)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Thresholds")
    float PeakZ = 1200.f;

    // Base colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor WaterColor = FColor(25, 60, 160, 255);      // blue

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor MountainColor = FColor(130, 130, 130, 255); // grey

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor PeakColor = FColor(245, 245, 245, 255);     // white

    // Contours
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Contours")
    FColor ShorelineColor = FColor(230, 230, 230, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Contours")
    bool bDrawPeakContour = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Contours")
    FColor PeakContourColor = FColor(255, 255, 255, 255);
};

UCLASS(BlueprintType)
class PERLINNOISEGEN_API UMapTextureGenerator : public UObject
{
    GENERATED_BODY()

public:
    // Generates and returns a transient texture (PF_B8G8R8A8)
    UFUNCTION(BlueprintCallable, Category = "Map")
    UTexture2D* GenerateMapTexture(ANoiseTerrainActor* Terrain, const FMapGenSettings& Settings);

    // World XY -> normalized UV in [0..1] relative to terrain bounds (centered)
    UFUNCTION(BlueprintCallable, Category = "Map")
    bool WorldToMapUV(const ANoiseTerrainActor* Terrain, float WorldX, float WorldY, float& OutU, float& OutV) const;

private:
    void PutPixelClamped(TArray<FColor>& Pixels, int32 W, int32 H, int32 X, int32 Y, const FColor& Color);
    void DrawLine(TArray<FColor>& Pixels, int32 W, int32 H, FVector2D A, FVector2D B, const FColor& Color, int32 Thickness);

    void RasterizeMarchingSquares(
        TArray<FColor>& Pixels, int32 W, int32 H,
        const TArray<uint8>& Field, int32 FX, int32 FY,
        const FColor& LineColor, int32 Thickness
    );

    FORCEINLINE int32 FieldIdx(int32 X, int32 Y, int32 FX) const { return Y * FX + X; }
};
