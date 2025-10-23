#include "NoiseTerrainActor.h"
#include "ProceduralMeshComponent.h"
#include "PerlinNoise.h"   
#include "DrawDebugHelpers.h"

ANoiseTerrainActor::ANoiseTerrainActor()
{
    PrimaryActorTick.bCanEverTick = false;

    ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    ProcMesh->bUseAsyncCooking = true;
    SetRootComponent(ProcMesh);

    // Allocate the noise generator
    NoisePtr = new FPerlinNoise(Seed);
}

void ANoiseTerrainActor::OnConstruction(const FTransform& Transform)
{
    if (!NoisePtr) NoisePtr = new FPerlinNoise(Seed);
    NoisePtr->reseed(Seed);
    BuildMesh();
}

void ANoiseTerrainActor::Regenerate()
{
    if (!NoisePtr) NoisePtr = new FPerlinNoise(Seed);
    NoisePtr->reseed(Seed);
    BuildMesh();
}

void ANoiseTerrainActor::BuildMesh()
{
    if (ProcMesh)
    {
        ProcMesh->ClearAllMeshSections();
        // If you ever add multiple sections, they’re gone now.
        // Collision will be re-cooked by CreateMeshSection below.
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    GenerateGrid(Vertices, Triangles, Normals, UVs, Tangents);

    ProcMesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UVs,
        TArray<FLinearColor>(),
        Tangents,
        bCreateCollision
    );

    if (TerrainMaterial)
    {
        ProcMesh->SetMaterial(0, TerrainMaterial);
    }

    if (bShowSlab && bEnableFlatten)
    {
        BuildSlabSection(); // creates section 1, no collision
    }

    // Water last so it renders on top where visible
    if (bShowWater)
    {
        BuildWaterSection(); // creates section 2, no collision
    }

    DebugDrawNormals(Vertices, Normals);
}


void ANoiseTerrainActor::GenerateGrid(
    TArray<FVector>& OutVertices,
    TArray<int32>& OutTriangles,
    TArray<FVector>& OutNormals,
    TArray<FVector2D>& OutUVs,
    TArray<FProcMeshTangent>& OutTangents
)
{
    check(NoisePtr);

    const int32 VertsX = NumQuadsX + 1;
    const int32 VertsY = NumQuadsY + 1;
    const int32 TotalVerts = VertsX * VertsY;

    const float HalfW = NumQuadsX * GridSpacing * 0.5f;
    const float HalfH = NumQuadsY * GridSpacing * 0.5f;

    OutVertices.SetNumUninitialized(TotalVerts);
    OutUVs.SetNumUninitialized(TotalVerts);

    // --- Heights: index-space sampling for smooth, small hills ---
    // Decouples noise frequency from centimeters; avoids "flat at spacing=200" issue.
    int32 Index = 0;
    for (int32 y = 0; y < VertsY; ++y)
    {
        for (int32 x = 0; x < VertsX; ++x, ++Index)
        {
            const float LocalX = x * GridSpacing - HalfW;  // centered around 0
            const float LocalY = y * GridSpacing - HalfH;

            // Index-space sampling with tiny offset to avoid lattice corners
            const float nx = (x + NoiseOffset.X) * FeatureScale;
            const float ny = (y + NoiseOffset.Y) * FeatureScale;

            const float h = NoisePtr->FBm2D(nx, ny, Octaves, Lacunarity, Persistence); // ~[-1,1]
            float Height = h * HeightAmplitude;

            // --- optional flatten pad with smooth rectangle falloff ---
            if (bEnableFlatten)
            {
                const float Cx = FlattenCenter.X;
                const float Cy = FlattenCenter.Y;
                const float hx = 0.5f * FlattenSize.X; // half extents
                const float hy = 0.5f * FlattenSize.Y;

                // Signed distance to axis-aligned rectangle (negative inside, positive outside)
                const float sx = FMath::Abs(LocalX - Cx) - hx;
                const float sy = FMath::Abs(LocalY - Cy) - hy;
                const float s = FMath::Max(sx, sy); // <= 0 inside the rectangle, > 0 outside

                // Map distance to a 0..1 feather factor with a smoothstep curve
                const float t = FMath::Clamp(s / FlattenFalloff, 0.f, 1.f);
                const float w = 1.f - (t * t * (3.f - 2.f * t)); // smoothstep(1 - t)

                // Lerp current height toward the plane height
                Height = FMath::Lerp(Height, FlattenHeight, w);
            }


            OutVertices[Index] = FVector(LocalX, LocalY, Height);
            OutUVs[Index] = FVector2D(
                static_cast<float>(x) / static_cast<float>(NumQuadsX),
                static_cast<float>(y) / static_cast<float>(NumQuadsY)
            );
        }
    }

    // --- Optional softening pass (one-iteration Laplacian-like) ---
    //if (bSmoothHeights)
    //{
    //    TArray<float> H; H.SetNumUninitialized(TotalVerts);
    //    for (int32 i = 0; i < TotalVerts; ++i) H[i] = OutVertices[i].Z;

    //    for (int32 y = 1; y < VertsY - 1; ++y)
    //    {
    //        for (int32 x = 1; x < VertsX - 1; ++x)
    //        {
    //            const int32 i = y * VertsX + x;
    //            const int32 l = i - 1, r = i + 1, u = i - VertsX, d = i + VertsX;
    //            OutVertices[i].Z = (H[i] + H[l] + H[r] + H[u] + H[d]) / 5.f;
    //        }
    //    }
    //}

    // --- Triangles (CCW, facing +Z) ---
    OutTriangles.Reset();
    OutTriangles.Reserve(NumQuadsX * NumQuadsY * 6);
    auto V = [VertsX](int32 X, int32 Y) { return Y * VertsX + X; };

    for (int32 y = 0; y < NumQuadsY; ++y)
    {
        for (int32 x = 0; x < NumQuadsX; ++x)
        {
            const int32 v00 = V(x, y);
            const int32 v10 = V(x + 1, y);
            const int32 v01 = V(x, y + 1);
            const int32 v11 = V(x + 1, y + 1);

            // Front faces up (+Z)
            OutTriangles.Add(v00); OutTriangles.Add(v11); OutTriangles.Add(v10);
            OutTriangles.Add(v00); OutTriangles.Add(v01); OutTriangles.Add(v11);
        }
    }

    // --- Fast, smooth area-weighted normals ---
    OutNormals.SetNumZeroed(TotalVerts);
    for (int32 t = 0; t < OutTriangles.Num(); t += 3)
    {
        const int32 ia = OutTriangles[t];
        const int32 ib = OutTriangles[t + 1];
        const int32 ic = OutTriangles[t + 2];

        const FVector& A = OutVertices[ia];
        const FVector& B = OutVertices[ib];
        const FVector& C = OutVertices[ic];

        const FVector FaceN = FVector::CrossProduct(C - A, B - A); // area-weighted
        OutNormals[ia] += FaceN;
        OutNormals[ib] += FaceN;
        OutNormals[ic] += FaceN;
    }
    
    /*for (FVector& N : OutNormals) { N.Normalize(); }*/
    for (FVector& N : OutNormals)
    {
        const double Len2 = N.SizeSquared();
        if (Len2 < 1e-12)
        {
            N = FVector::UpVector; // fallback to +Z normal if degenerate
        }
        else
        {
            N /= FMath::Sqrt(Len2); // normalize safely
        }
    }


    // --- Simple tangents (+X). Good for most world-aligned materials. ---
    OutTangents.SetNumUninitialized(TotalVerts);
    for (int32 i = 0; i < TotalVerts; ++i)
    {
        OutTangents[i] = FProcMeshTangent(1.f, 0.f, 0.f);
    }
}


void ANoiseTerrainActor::DebugDrawNormals(const TArray<FVector>& Vertices,
    const TArray<FVector>& Normals)
{
 
    UWorld* World = GetWorld();
    if (!World) return;

    // Clear previous persistent lines so you see the latest
    FlushPersistentDebugLines(World);

    if (!bDebugDrawNormals) return;

    const FTransform T = GetActorTransform();

    // Draw up to ~512 samples for clarity
    const int32 Step = FMath::Max(1, Vertices.Num() / 512);

    for (int32 i = 0; i < Vertices.Num(); i += Step)
    {
        const FVector P = T.TransformPosition(Vertices[i]);
        const FVector N = T.TransformVectorNoScale(Normals[i]).GetSafeNormal();

        // Persistent=true, LifeTime=30s so you can see them
        DrawDebugLine(World, P, P + N * DebugNormalLength,
            FColor::Cyan, /*bPersistentLines=*/true,
            /*LifeTime=*/30.f, /*DepthPriority=*/0, /*Thickness=*/2.f);
    }
}

void ANoiseTerrainActor::BuildSlabSection()
{
    // Compute slab extents from your flatten params
    const float hx = 0.5f * FMath::Max(0.f, FlattenSize.X - 2.f * SlabInset);
    const float hy = 0.5f * FMath::Max(0.f, FlattenSize.Y - 2.f * SlabInset);

    if (hx <= 0.f || hy <= 0.f) return;

    const float zTop = FlattenHeight + SlabZOffset;

    // 4 verts (CCW, +Z up)
    TArray<FVector> V;
    V.Reserve(4);
    V.Add(FVector(FlattenCenter.X - hx, FlattenCenter.Y - hy, zTop)); // BL
    V.Add(FVector(FlattenCenter.X + hx, FlattenCenter.Y - hy, zTop)); // BR
    V.Add(FVector(FlattenCenter.X + hx, FlattenCenter.Y + hy, zTop)); // TR
    V.Add(FVector(FlattenCenter.X - hx, FlattenCenter.Y + hy, zTop)); // TL

    TArray<int32> I = { 0, 2, 1, 0, 3, 2 };

    // Normals up, simple UVs 0..1
    TArray<FVector> N; N.Init(FVector::UpVector, 4);
    TArray<FVector2D> UV; UV.Reserve(4);
    UV.Add(FVector2D(0.f, 0.f));
    UV.Add(FVector2D(1.f, 0.f));
    UV.Add(FVector2D(1.f, 1.f));
    UV.Add(FVector2D(0.f, 1.f));

    TArray<FProcMeshTangent> T; T.Init(FProcMeshTangent(1, 0, 0), 4);

    // Section 1, NO collision
    ProcMesh->CreateMeshSection_LinearColor(
        1, V, I, N, UV, TArray<FLinearColor>(), T, /*bCreateCollision=*/false);

    if (SlabMaterial) ProcMesh->SetMaterial(1, SlabMaterial);
}


void ANoiseTerrainActor::BuildWaterSection()
{
    // Terrain half extents in local space
    const float HalfW = NumQuadsX * GridSpacing * 0.5f + WaterPadding;
    const float HalfH = NumQuadsY * GridSpacing * 0.5f + WaterPadding;

    const float Z = WaterZ + WaterZOffset;

    // Quad corners (CCW, +Z up), centered like your terrain
    TArray<FVector> V;
    V.Reserve(4);
    V.Add(FVector(-HalfW, -HalfH, Z)); // BL
    V.Add(FVector(+HalfW, -HalfH, Z)); // BR
    V.Add(FVector(+HalfW, +HalfH, Z)); // TR
    V.Add(FVector(-HalfW, +HalfH, Z)); // TL

    TArray<int32> I = { 0, 2, 1, 0, 3, 2 };

    // Up normals
    TArray<FVector> N; N.Init(FVector::UpVector, 4);

    // UVs with tiling (0..WaterUVTile)
    const float UMax = WaterUVTile;
    const float VMax = WaterUVTile;
    TArray<FVector2D> UV; UV.Reserve(4);
    UV.Add(FVector2D(0.f, 0.f));
    UV.Add(FVector2D(UMax, 0.f));
    UV.Add(FVector2D(UMax, VMax));
    UV.Add(FVector2D(0.f, VMax));

    TArray<FProcMeshTangent> T; T.Init(FProcMeshTangent(1, 0, 0), 4);

    // Section 2, NO collision
    ProcMesh->CreateMeshSection_LinearColor(
        2, V, I, N, UV, TArray<FLinearColor>(), T, /*bCreateCollision=*/false);

    if (WaterMaterial) { ProcMesh->SetMaterial(2, WaterMaterial); }

    // Optional: hide shadows on a translucent surface (engine usually ignores anyway)
    ProcMesh->bCastDynamicShadow = false;
}
