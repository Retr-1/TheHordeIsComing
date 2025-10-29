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
    HeightCache.SetNumUninitialized(VertsX * VertsY);

    // --- Heights: index-space sampling for smooth, small hills ---
    // Decouples noise frequency from centimeters; avoids "flat at spacing=200" issue.
    int32 Index = 0;
    for (int32 y = 0; y < VertsY; ++y)
    {
        for (int32 x = 0; x < VertsX; ++x, ++Index)
        {
            const float LocalX = x * GridSpacing - HalfW;  // centered
            const float LocalY = y * GridSpacing - HalfH;

            // Use the unified function
            const float Height = SampleHeightAtIndex(x, y, LocalX, LocalY);

            OutVertices[Index] = FVector(LocalX, LocalY, Height);
            OutUVs[Index] = FVector2D(
                (float)x / (float)NumQuadsX,
                (float)y / (float)NumQuadsY
            );
            HeightCache[Index] = Height;
        }
    }
    bCacheValid = true;


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

float ANoiseTerrainActor::SampleHeightAtIndex(int32 ix, int32 iy, float LocalX, float LocalY) const
{
    // Noise in *index* space — matches GenerateGrid
    const float nx = (ix + NoiseOffset.X) * FeatureScale;
    const float ny = (iy + NoiseOffset.Y) * FeatureScale;
    const float hNoise = NoisePtr ? NoisePtr->FBm2D(nx, ny, Octaves, Lacunarity, Persistence) : 0.f; // ~[-1,1]
    float Height = hNoise * HeightAmplitude;

    if (bEnableFlatten)
    {
        const float Cx = FlattenCenter.X;
        const float Cy = FlattenCenter.Y;
        const float hx = 0.5f * FlattenSize.X;
        const float hy = 0.5f * FlattenSize.Y;

        const float sx = FMath::Abs(LocalX - Cx) - hx;
        const float sy = FMath::Abs(LocalY - Cy) - hy;
        const float s = FMath::Max(sx, sy); // <= 0 inside rectangle

        const float falloff = FMath::Max(FlattenFalloff, 1.f); // avoid div by 0
        const float t = FMath::Clamp(s / falloff, 0.f, 1.f);
        const float w = 1.f - Smoothstep01(t); // 1 inside, 0 outside

        Height = FMath::Lerp(Height, FlattenHeight, w);
    }
    return Height;
}

float ANoiseTerrainActor::HeightAtLocalXY(float LocalX, float LocalY, bool bClampToBounds) const
{
    const int32 VertsX = NumQuadsX + 1;
    const int32 VertsY = NumQuadsY + 1;

    const float HalfW = NumQuadsX * GridSpacing * 0.5f;
    const float HalfH = NumQuadsY * GridSpacing * 0.5f;

    float u = (LocalX + HalfW) / GridSpacing;
    float v = (LocalY + HalfH) / GridSpacing;

    if (bClampToBounds) {
        u = FMath::Clamp(u, 0.f, (float)NumQuadsX);
        v = FMath::Clamp(v, 0.f, (float)NumQuadsY);
    }
    else {
        if (u < 0.f || u > NumQuadsX || v < 0.f || v > NumQuadsY) return 0.f;
    }

    const int32 ix = FMath::Clamp(FMath::FloorToInt(u), 0, NumQuadsX - 1);
    const int32 iy = FMath::Clamp(FMath::FloorToInt(v), 0, NumQuadsY - 1);
    const float  tx = u - (float)ix;
    const float  ty = v - (float)iy;

    if (bCacheValid && HeightCache.Num() == VertsX * VertsY)
    {
        const int32 i00 = CacheIndex(ix, iy, VertsX);
        const int32 i10 = CacheIndex(ix + 1, iy, VertsX);
        const int32 i01 = CacheIndex(ix, iy + 1, VertsX);
        const int32 i11 = CacheIndex(ix + 1, iy + 1, VertsX);

        const float h00 = HeightCache[i00];
        const float h10 = HeightCache[i10];
        const float h01 = HeightCache[i01];
        const float h11 = HeightCache[i11];

        const float hx0 = FMath::Lerp(h00, h10, tx);
        const float hx1 = FMath::Lerp(h01, h11, tx);
        return FMath::Lerp(hx0, hx1, ty);
    }

    // Fallback (no cache): exact computation
    const float x0 = ix * GridSpacing - HalfW;
    const float y0 = iy * GridSpacing - HalfH;
    const float x1 = (ix + 1) * GridSpacing - HalfW;
    const float y1 = (iy + 1) * GridSpacing - HalfH;

    const float h00 = SampleHeightAtIndex(ix, iy, x0, y0);
    const float h10 = SampleHeightAtIndex(ix + 1, iy, x1, y0);
    const float h01 = SampleHeightAtIndex(ix, iy + 1, x0, y1);
    const float h11 = SampleHeightAtIndex(ix + 1, iy + 1, x1, y1);

    const float hx0 = FMath::Lerp(h00, h10, tx);
    const float hx1 = FMath::Lerp(h01, h11, tx);
    return FMath::Lerp(hx0, hx1, ty);
}


float ANoiseTerrainActor::GetHeightAtWorldXY(float WorldX, float WorldY, bool bClampToBounds) const
{
    const FTransform& T = GetActorTransform();
    const FVector L = T.InverseTransformPosition(FVector(WorldX, WorldY, 0.f));
    return HeightAtLocalXY(L.X, L.Y, bClampToBounds);
}


FVector ANoiseTerrainActor::GetNormalAtWorldXY(float WorldX, float WorldY, bool bClampToBounds) const
{
    if (GridSpacing <= 0.f) return FVector::UpVector;

    const float hC = GetHeightAtWorldXY(WorldX, WorldY, bClampToBounds);

    const FVector P(WorldX, WorldY, 0.f);
    const FVector RightW = P + FVector(GridSpacing, 0.f, 0.f);
    const FVector LeftW = P - FVector(GridSpacing, 0.f, 0.f);
    const FVector FwdW = P + FVector(0.f, GridSpacing, 0.f);
    const FVector BackW = P - FVector(0.f, GridSpacing, 0.f);

    const float hR = GetHeightAtWorldXY(RightW.X, RightW.Y, bClampToBounds);
    const float hL = GetHeightAtWorldXY(LeftW.X, LeftW.Y, bClampToBounds);
    const float hF = GetHeightAtWorldXY(FwdW.X, FwdW.Y, bClampToBounds);
    const float hB = GetHeightAtWorldXY(BackW.X, BackW.Y, bClampToBounds);

    const FVector dX(2.f * GridSpacing, 0.f, hR - hL);
    const FVector dY(0.f, 2.f * GridSpacing, hF - hB);

    FVector N = FVector::CrossProduct(dY, dX);
    const double len2 = N.SizeSquared();
    return (len2 < 1e-12) ? FVector::UpVector : N / FMath::Sqrt(len2);
}
