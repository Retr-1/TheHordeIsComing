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

    // Output texture resolution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution", meta = (ClampMin = "16", UIMin = "16"))
    int32 MapWidth = 512;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Resolution", meta = (ClampMin = "16", UIMin = "16"))
    int32 MapHeight = 512;

    // Optional: speed-up for big textures (uses multicore)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Performance")
    bool bUseParallelFor = true;

    // Colors (you asked for water/grass/rock/peaks using actor thresholds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor WaterColor = FColor(25, 60, 160, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor GrassColor = FColor(40, 110, 45, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor RockColor = FColor(130, 130, 130, 255);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Colors")
    FColor PeakColor = FColor(245, 245, 245, 255);

    // If you want slightly nicer band edges without extra sampling
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Style", meta = (ClampMin = "0", UIMin = "0"))
    int32 BandDitherStrength = 0; // 0 = off (fastest)
};

UCLASS(BlueprintType)
class PERLINNOISEGEN_API UMapTextureGenerator : public UObject
{
    GENERATED_BODY()

public:
    // Generates a transient PF_B8G8R8A8 texture using only per-pixel sampling (no marching squares)
    UFUNCTION(BlueprintCallable, Category = "Map")
    UTexture2D* GenerateMapTexture(ANoiseTerrainActor* Terrain, const FMapGenSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Map")
    bool WorldToMapUV(const ANoiseTerrainActor* Terrain, float WorldX, float WorldY, float& OutU, float& OutV) const;


private:
    FORCEINLINE int32 PixelIndex(int32 X, int32 Y, int32 W) const { return Y * W + X; }
    static uint8 Hash8(int32 x, int32 y);
};
