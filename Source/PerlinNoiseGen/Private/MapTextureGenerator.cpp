#include "MapTextureGenerator.h"
#include "Engine/Texture2D.h"
#include "NoiseTerrainActor.h"

#include "Async/ParallelFor.h"

uint8 UMapTextureGenerator::Hash8(int32 x, int32 y)
{
    // tiny deterministic hash for optional dithering
    uint32 h = (uint32)x * 374761393u + (uint32)y * 668265263u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    h ^= (h >> 16u);
    return (uint8)(h & 0xFFu);
}

bool UMapTextureGenerator::WorldToMapUV_LocalTerrain(const ANoiseTerrainActor* Terrain, float WorldX, float WorldY, float& OutU, float& OutV) const
{
    if (!Terrain) return false;

    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;
    if (HalfW <= 0.f || HalfH <= 0.f) return false;

    // Terrain is centered at origin => world == local (your assumption)
    const float LocalX = WorldX;
    const float LocalY = WorldY;

    OutU = (LocalX + HalfW) / (2.f * HalfW);
    OutV = (LocalY + HalfH) / (2.f * HalfH);

    OutU = FMath::Clamp(OutU, 0.f, 1.f);
    OutV = FMath::Clamp(OutV, 0.f, 1.f);
    return true;
}

UTexture2D* UMapTextureGenerator::GenerateMapTexture(ANoiseTerrainActor* Terrain, const FMapGenSettings& Settings)
{
    if (!Terrain) return nullptr;

    const int32 W = Settings.MapWidth;
    const int32 H = Settings.MapHeight;
    if (W < 2 || H < 2) return nullptr;

    // Terrain extents in LOCAL space (centered)
    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;

    // Thresholds live on the terrain actor (you said you added these)
    const float WaterZ = Terrain->WaterZ;
    const float GrassTop = Terrain->GrassTop;
    const float RockTop = Terrain->RockTop;

    // Precompute local X per column and local Y per row to avoid lerp in inner loop
    TArray<float> LocalXs;
    TArray<float> LocalYs;
    LocalXs.SetNumUninitialized(W);
    LocalYs.SetNumUninitialized(H);

    for (int32 px = 0; px < W; ++px)
    {
        const float u = (float)px / (float)(W - 1);
        LocalXs[px] = FMath::Lerp(-HalfW, +HalfW, u);
    }
    for (int32 py = 0; py < H; ++py)
    {
        const float v = (float)py / (float)(H - 1);
        LocalYs[py] = FMath::Lerp(-HalfH, +HalfH, v);
    }

    // Pixel buffer
    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(W * H);

    auto ShadeHeight = [&](float Z, int32 px, int32 py) -> FColor
        {
            // Optional tiny dithering around thresholds (cheap visual improvement)
            float z = Z;
            if (Settings.BandDitherStrength > 0)
            {
                const uint8 r = Hash8(px, py);
                const float n = ((float)r / 255.f) * 2.f - 1.f; // [-1..1]
                z += n * (float)Settings.BandDitherStrength;
            }

            if (z < WaterZ)      return Settings.WaterColor;
            if (z < GrassTop)    return Settings.GrassColor;
            if (z < RockTop)     return Settings.RockColor;
            return Settings.PeakColor;
        };

    auto RowJob = [&](int32 py)
        {
            const float LocalY = LocalYs[py];
            const int32 RowBase = py * W;

            for (int32 px = 0; px < W; ++px)
            {
                const float LocalX = LocalXs[px];

                // **Fast cached bilinear sample** (this is why we exposed GetHeightAtLocalXY)
                const float Z = Terrain->HeightAtLocalXY(LocalX, LocalY, /*bClampToBounds=*/true);

                Pixels[RowBase + px] = ShadeHeight(Z, px, py);
            }
        };

    if (Settings.bUseParallelFor)
    {
        ParallelFor(H, [&](int32 py) { RowJob(py); });
    }
    else
    {
        for (int32 py = 0; py < H; ++py) RowJob(py);
    }

    // Upload to transient texture
    UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
    if (!Tex) return nullptr;

    Tex->MipGenSettings = TMGS_NoMipmaps;
    Tex->CompressionSettings = TC_VectorDisplacementmap; // crisp
    Tex->SRGB = true;
    Tex->NeverStream = true;

    if (!Tex->GetPlatformData() || Tex->GetPlatformData()->Mips.Num() == 0)
    {
        return Tex;
    }

    void* TextureData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Tex->GetPlatformData()->Mips[0].BulkData.Unlock();

    Tex->UpdateResource();
    return Tex;
}
