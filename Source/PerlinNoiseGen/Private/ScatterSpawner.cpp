#include "ScatterSpawner.h"
#include "NoiseTerrainActor.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

AScatterSpawner::AScatterSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AScatterSpawner::OnConstruction(const FTransform& /*Xform*/)
{
    // no-op; use editor buttons
}

void AScatterSpawner::ClearSpawned()
{
    for (auto& W : SpawnedActors)
    {
        if (AActor* A = W.Get())
        {
#if WITH_EDITOR
            A->Modify();
#endif
            A->Destroy();
        }
    }
    SpawnedActors.Reset();
}

void AScatterSpawner::Generate()
{
    if (!Terrain)
    {
        UE_LOG(LogTemp, Warning, TEXT("ScatterSpawner: Terrain is null."));
        return;
    }

    // Optional: clear previous
    ClearSpawned();

    // Terrain extents in local space
    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;

    FVector2D LocMin(-HalfW, -HalfH);
    FVector2D LocMax(+HalfW, +HalfH);

    if (bUseRegion)
    {
        LocMin.X = FMath::Clamp(RegionMin_Local.X, -HalfW, +HalfW);
        LocMin.Y = FMath::Clamp(RegionMin_Local.Y, -HalfH, +HalfH);
        LocMax.X = FMath::Clamp(RegionMax_Local.X, -HalfW, +HalfW);
        LocMax.Y = FMath::Clamp(RegionMax_Local.Y, -HalfH, +HalfH);
        if (LocMax.X < LocMin.X) Swap(LocMax.X, LocMin.X);
        if (LocMax.Y < LocMin.Y) Swap(LocMax.Y, LocMin.Y);
    }

    FRandomStream RNG(Seed);

    for (const FSpawnRequest& R : Requests)
    {
        if (!R.ActorClass) continue;

        TArray<FVector2D> Placed2D;
        Placed2D.Reserve(R.Count);

        int32 Spawned = 0;
        int32 Tries = 0;
        const int32 MaxTries = FMath::Max(1, R.MaxTriesPerInstance) * FMath::Max(1, R.Count);

        while (Spawned < R.Count && Tries < MaxTries)
        {
            ++Tries;

            // Random local XY on terrain (or constrained region)
            const float rx = RNG.FRandRange(LocMin.X, LocMax.X);
            const float ry = RNG.FRandRange(LocMin.Y, LocMax.Y);

            // Local -> World (XY)
            const FVector WorldOnPlane = Terrain->GetActorTransform().TransformPosition(FVector(rx, ry, 0.f));

            float z = 0.f;
            FVector n = FVector::UpVector;

            if (!AcceptByConstraints(R, WorldOnPlane.X, WorldOnPlane.Y, z, n))
                continue;

            if (!RespectSpacing(R, WorldOnPlane.X, WorldOnPlane.Y, Placed2D))
                continue;

            // Random spin around the surface normal
            const float SpinDeg = R.bRandomYaw ? RNG.FRandRange(0.f, 360.f) : 0.f;
            const float ScaleU = FMath::FRandRange(R.UniformScaleRange.X, R.UniformScaleRange.Y);

            // Ensure the normal is normalized
            const FVector N = n.GetSafeNormal();

            // Lift along the normal to avoid clipping on slopes
            const FVector Loc = FVector(WorldOnPlane.X, WorldOnPlane.Y, z) + N * R.SurfaceOffset;

            // Build rotation: align actor's +Z to the surface normal, then spin around that normal
            const FQuat AlignQuat = FRotationMatrix::MakeFromZ(N).ToQuat();
            const FQuat SpinQuat = FQuat(N, FMath::DegreesToRadians(SpinDeg));
            const FQuat FinalQuat = SpinQuat * AlignQuat;

            FTransform T;
            T.SetLocation(Loc);
            T.SetRotation(FinalQuat);
            T.SetScale3D(FVector(ScaleU));


            FActorSpawnParameters P;
            P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            if (AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(R.ActorClass, T, P))
            {
                SpawnedActors.Add(SpawnedActor);
                Placed2D.Add(FVector2D(WorldOnPlane.X, WorldOnPlane.Y));
                ++Spawned;
            }
        }

        UE_LOG(LogTemp, Log, TEXT("ScatterSpawner: %d/%d spawned for %s (tries=%d)"),
            Spawned, R.Count, *R.ActorClass->GetName(), Tries);
    }
}

bool AScatterSpawner::AcceptByConstraints(const FSpawnRequest& R, float X, float Y, float& OutZ, FVector& OutNormal) const
{
    if (!Terrain) return false;

    const float z = Terrain->GetHeightAtWorldXY(X, Y, /*bClampToBounds*/true);
    OutZ = z;

    // Z window
    if (z < R.MinZ || z > R.MaxZ) return false;

    // Optional: below water rejection
    if (R.bDisallowBelowWater && z < Terrain->WaterZ) return false;

    // Always compute normal (we use it for alignment)
    OutNormal = Terrain->GetNormalAtWorldXY(X, Y, /*bClamp*/true).GetSafeNormal();
    OutNormal.Normalize();
    if (OutNormal.Z < 0.f)
    {
        OutNormal *= -1.f;  // force up-facing
    }

    // Slope constraint (optional)
    if (R.MinSlopeDeg > 0.f || R.MaxSlopeDeg < 90.f)
    {
        const float slopeRad = FMath::Acos(FMath::Clamp(OutNormal.Z, -1.f, 1.f));
        const float slopeDeg = FMath::RadiansToDegrees(slopeRad);
        if (slopeDeg < R.MinSlopeDeg || slopeDeg > R.MaxSlopeDeg) return false;
    }

    return true;
}


bool AScatterSpawner::RespectSpacing(const FSpawnRequest& R, float X, float Y, const TArray<FVector2D>& Placed2D) const
{
    if (R.MinSpacing <= 0.f) return true;
    const float MinDist2 = R.MinSpacing * R.MinSpacing;

    const FVector2D p(X, Y);
    for (const FVector2D& q : Placed2D)
    {
        if (FVector2D::DistSquared(p, q) < MinDist2)
            return false;
    }
    return true;
}

// (Unused right now, but kept for future region pick logic customizations)
bool AScatterSpawner::PickRandomXY(FRandomStream& RNG, float& OutX, float& OutY) const
{
    if (!Terrain) return false;
    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;

    const float minX = bUseRegion ? FMath::Clamp(RegionMin_Local.X, -HalfW, +HalfW) : -HalfW;
    const float maxX = bUseRegion ? FMath::Clamp(RegionMax_Local.X, -HalfW, +HalfW) : +HalfW;
    const float minY = bUseRegion ? FMath::Clamp(RegionMin_Local.Y, -HalfH, +HalfH) : -HalfH;
    const float maxY = bUseRegion ? FMath::Clamp(RegionMax_Local.Y, -HalfH, +HalfH) : +HalfH;

    OutX = RNG.FRandRange(minX, maxX);
    OutY = RNG.FRandRange(minY, maxY);
    return true;
}
