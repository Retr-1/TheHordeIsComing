#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NoiseTerrainActor.generated.h"

// Forward declarations to keep the public header light
class UProceduralMeshComponent;
struct FProcMeshTangent;
class FPerlinNoise;

UCLASS()
class PERLINNOISEGEN_API ANoiseTerrainActor : public AActor
{
    GENERATED_BODY()

public:
    ANoiseTerrainActor();

    // ---- Components ----
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProceduralMeshComponent* ProcMesh;

    // ---- Grid ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "1", UIMin = "1"))
    int32 NumQuadsX = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "1", UIMin = "1"))
    int32 NumQuadsY = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Grid", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float GridSpacing = 100.f;

    // ---- Noise ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise")
    float HeightAmplitude = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "1", UIMin = "1"))
    int32 Octaves = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "0.0001", UIMin = "1.0"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float Persistence = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise")
    int32 Seed = 1337;

    // Sample in index space (x,y) * FeatureScale for smoother, predictable hills
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise", meta = (ClampMin = "0.0001", UIMin = "0.0001"))
    float FeatureScale = 0.0125f;

    // Small offsets to avoid lattice lock; you can tweak, or seed them
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise")
    FVector2D NoiseOffset = FVector2D(37.123f, 53.789f);

    // Optional single-pass height smoothing to tame razor peaks
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Noise")
    //bool bSmoothHeights = false;

    // ---- Mesh ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Mesh")
    bool bCreateCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Mesh")
    UMaterialInterface* TerrainMaterial = nullptr;

    // Rebuild (shows as a button in Details panel)
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Terrain")
    void Regenerate();

    UPROPERTY(EditAnywhere, Category = "Terrain|Debug")
    bool bDebugDrawNormals = true;

    UPROPERTY(EditAnywhere, Category = "Terrain|Debug", meta = (ClampMin = "10", UIMin = "10"))
    float DebugNormalLength = 300.f;

    void DebugDrawNormals(const TArray<FVector>& Vertices,
        const TArray<FVector>& Normals);


    UPROPERTY(EditAnywhere, Category = "Terrain|Flatten")
    bool bEnableFlatten = true;

    UPROPERTY(EditAnywhere, Category = "Terrain|Flatten")
    FVector2D FlattenCenter = FVector2D(0.f, 0.f);   // cm, center of the pad in world XY

    UPROPERTY(EditAnywhere, Category = "Terrain|Flatten")
    FVector2D FlattenSize = FVector2D(5000.f, 5000.f); // cm, width & height of the rectangle

    UPROPERTY(EditAnywhere, Category = "Terrain|Flatten")
    float FlattenHeight = 0.f; // Z of the flat area (0 = at world origin height)

    UPROPERTY(EditAnywhere, Category = "Terrain|Flatten", meta = (ClampMin = "1", UIMin = "1"))
    float FlattenFalloff = 800.f; // cm of smooth feathering from the edge outward

    // ---- Slab (visual only) ----
    UPROPERTY(EditAnywhere, Category = "Terrain|Slab")
    bool bShowSlab = true;

    UPROPERTY(EditAnywhere, Category = "Terrain|Slab", meta = (ClampMin = "0.0"))
    float SlabThickness = 8.f;          // only used for the "thin box" option

    UPROPERTY(EditAnywhere, Category = "Terrain|Slab")
    float SlabZOffset = 1.0f;           // lift to avoid z-fighting with the flat pad

    UPROPERTY(EditAnywhere, Category = "Terrain|Slab", meta = (ClampMin = "0.0"))
    float SlabInset = 20.f;             // shrink a bit so it stays inside the blended edge

    UPROPERTY(EditAnywhere, Category = "Terrain|Slab")
    UMaterialInterface* SlabMaterial = nullptr;

    // ---- Water (visual only) ----
    UPROPERTY(EditAnywhere, Category = "Terrain|Water")
    bool bShowWater = true;

    UPROPERTY(EditAnywhere, Category = "Terrain|Water")
    float WaterZ = 0.f;                      // flood level (Z)

    UPROPERTY(EditAnywhere, Category = "Terrain|Water", meta = (ClampMin = "0.0"))
    float WaterPadding = 200.f;              // extend beyond terrain bounds to hide edges

    UPROPERTY(EditAnywhere, Category = "Terrain|Water", meta = (ClampMin = "0.1"))
    float WaterUVTile = 1.0f;                // UV tiling factor for ripples

    UPROPERTY(EditAnywhere, Category = "Terrain|Water", meta = (ClampMin = "0.0"))
    float WaterZOffset = 0.5f;               // tiny lift to avoid coplanar z-fight at shores

    UPROPERTY(EditAnywhere, Category = "Terrain|Water")
    UMaterialInterface* WaterMaterial = nullptr;




protected:
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void BuildMesh();

    void GenerateGrid(
        TArray<FVector>& OutVertices,
        TArray<int32>& OutTriangles,
        TArray<FVector>& OutNormals,
        TArray<FVector2D>& OutUVs,
        TArray<FProcMeshTangent>& OutTangents
    );

    void BuildSlabSection();
    void BuildWaterSection();

    // Seedable Perlin noise (header-only helper)
    FPerlinNoise* NoisePtr = nullptr;
};
