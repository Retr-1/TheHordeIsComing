#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"   // add this near the top
#include "GameFramework/Actor.h"
#include "ScatterSpawner.generated.h"

class ANoiseTerrainActor;

USTRUCT(BlueprintType)
struct FSpawnRequest
{
    GENERATED_BODY()

    // What to spawn
    UPROPERTY(EditAnywhere, Category = "Spawn")
    TSubclassOf<AActor> ActorClass = nullptr;

    // How many instances
    UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0"))
    int32 Count = 100;

    // Z constraints (inclusive)
    UPROPERTY(EditAnywhere, Category = "Constraints")
    float MinZ = -FLT_MAX;

    UPROPERTY(EditAnywhere, Category = "Constraints")
    float MaxZ = FLT_MAX;

    // Optional slope constraints (deg)
    UPROPERTY(EditAnywhere, Category = "Constraints", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MinSlopeDeg = 0.f;

    UPROPERTY(EditAnywhere, Category = "Constraints", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxSlopeDeg = 90.f;

    // Optional spacing (disable <= 0)
    UPROPERTY(EditAnywhere, Category = "Constraints", meta = (ClampMin = "0.0"))
    float MinSpacing = 0.f;

    // Lift above ground
    UPROPERTY(EditAnywhere, Category = "Placement")
    float SurfaceOffset = 0.f;

    // Random yaw
    UPROPERTY(EditAnywhere, Category = "Placement")
    bool bRandomYaw = true;

    // Uniform scale range
    UPROPERTY(EditAnywhere, Category = "Placement")
    FVector2D UniformScaleRange = FVector2D(1.f, 1.f);

    // Attempts per instance
    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (ClampMin = "1"))
    int32 MaxTriesPerInstance = 25;

    // Optional: don’t place below terrain’s WaterZ
    UPROPERTY(EditAnywhere, Category = "Constraints")
    bool bDisallowBelowWater = false;

    // Keep spawns off the terrain's central platform (core only)
    UPROPERTY(EditAnywhere, Category = "Constraints|Flatten")
    bool bDisallowOnFlattenCore = false;

    // Extra inflation (cm) applied to each half-extent of the core rectangle.
    // 0 = exactly the platform size from the terrain.
    UPROPERTY(EditAnywhere, Category = "Constraints|Flatten", meta = (ClampMin = "0.0"))
    float FlattenCoreExtra = 0.f;


};

UCLASS()
class PERLINNOISEGEN_API AScatterSpawner : public AActor
{
    GENERATED_BODY()

public:
    AScatterSpawner();

    // Terrain to query
    UPROPERTY(EditAnywhere, Category = "Terrain")
    ANoiseTerrainActor* Terrain = nullptr;

    // Deterministic placement
    UPROPERTY(EditAnywhere, Category = "Random")
    int32 Seed = 12345;

    // Restrict to a local (terrain-space) rectangle
    UPROPERTY(EditAnywhere, Category = "Region")
    bool bUseRegion = false;

    UPROPERTY(EditAnywhere, Category = "Region")
    FVector2D RegionMin_Local = FVector2D(-10000, -10000);

    UPROPERTY(EditAnywhere, Category = "Region")
    FVector2D RegionMax_Local = FVector2D(10000, 10000);

    // Multiple spawn batches (trees, chests, …)
    UPROPERTY(EditAnywhere, Category = "Spawn")
    TArray<FSpawnRequest> Requests;

    UFUNCTION(CallInEditor, Category = "Spawn")
    void Generate();

    UFUNCTION(CallInEditor, Category = "Spawn")
    void ClearSpawned();

protected:
    virtual void OnConstruction(const FTransform& Xform) override;

private:
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AActor>> SpawnedActors;

    bool PickRandomXY(FRandomStream& RNG, float& OutX, float& OutY) const;
    bool AcceptByConstraints(const FSpawnRequest& R, float X, float Y, float& OutZ, FVector& OutNormal) const;
    bool RespectSpacing(const FSpawnRequest& R, float X, float Y, const TArray<FVector2D>& Placed2D) const;
};
