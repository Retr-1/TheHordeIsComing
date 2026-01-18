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

    // Texture resolution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 MapWidth = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 MapHeight = 512;

    // Marching squares resolution (cells). Field is (CellsX+1)x(CellsY+1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 CellsX = 256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 CellsY = 256;

    // Shoreline drawing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    int32 LineThickness = 2;

    // Base fill colors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FColor LandColor = FColor(35, 45, 35, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FColor WaterColor = FColor(20, 30, 60, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
    FColor ShorelineColor = FColor(220, 220, 220, 255);
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
