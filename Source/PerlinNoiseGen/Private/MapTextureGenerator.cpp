// ===============================
// MapTextureGenerator.cpp
// ===============================
#include "MapTextureGenerator.h"
#include "Engine/Texture2D.h"
#include "NoiseTerrainActor.h"

static FORCEINLINE uint8 Bit(bool b) { return b ? 1 : 0; }

void UMapTextureGenerator::PutPixelClamped(TArray<FColor>& Pixels, int32 W, int32 H, int32 X, int32 Y, const FColor& Color)
{
    if (X < 0 || Y < 0 || X >= W || Y >= H) return;
    Pixels[Y * W + X] = Color;
}

void UMapTextureGenerator::DrawLine(TArray<FColor>& Pixels, int32 W, int32 H, FVector2D A, FVector2D B, const FColor& Color, int32 Thickness)
{
    const float dx = B.X - A.X;
    const float dy = B.Y - A.Y;
    const float steps = FMath::Max(FMath::Abs(dx), FMath::Abs(dy));
    if (steps <= 0.0f) return;

    const float xInc = dx / steps;
    const float yInc = dy / steps;

    float x = A.X;
    float y = A.Y;

    const int32 HalfT = FMath::Max(0, Thickness / 2);

    for (int32 i = 0; i <= (int32)steps; ++i)
    {
        const int32 xi = FMath::RoundToInt(x);
        const int32 yi = FMath::RoundToInt(y);

        for (int32 oy = -HalfT; oy <= HalfT; ++oy)
        {
            for (int32 ox = -HalfT; ox <= HalfT; ++ox)
            {
                PutPixelClamped(Pixels, W, H, xi + ox, yi + oy, Color);
            }
        }

        x += xInc;
        y += yInc;
    }
}

void UMapTextureGenerator::RasterizeMarchingSquares(
    TArray<FColor>& Pixels, int32 W, int32 H,
    const TArray<uint8>& Field, int32 FX, int32 FY,
    const FColor& LineColor, int32 Thickness)
{
    auto P = [&](int32 sx, int32 sy) -> FVector2D
        {
            const float u = (float)sx / (float)(FX - 1);
            const float v = (float)sy / (float)(FY - 1);
            return FVector2D(u * (W - 1), v * (H - 1));
        };

    for (int32 y = 0; y < FY - 1; ++y)
    {
        for (int32 x = 0; x < FX - 1; ++x)
        {
            const bool a = Field[FieldIdx(x, y, FX)] != 0; // top-left
            const bool b = Field[FieldIdx(x + 1, y, FX)] != 0; // top-right
            const bool c = Field[FieldIdx(x + 1, y + 1, FX)] != 0; // bottom-right
            const bool d = Field[FieldIdx(x, y + 1, FX)] != 0; // bottom-left

            const uint8 idx =
                (Bit(a) << 3) |
                (Bit(b) << 2) |
                (Bit(c) << 1) |
                (Bit(d) << 0);

            if (idx == 0 || idx == 15) continue;

            const FVector2D pa = P(x, y);
            const FVector2D pb = P(x + 1, y);
            const FVector2D pc = P(x + 1, y + 1);
            const FVector2D pd = P(x, y + 1);

            const FVector2D top = (pa + pb) * 0.5f;
            const FVector2D right = (pb + pc) * 0.5f;
            const FVector2D bottom = (pd + pc) * 0.5f;
            const FVector2D left = (pa + pd) * 0.5f;

            // Standard marching squares segment table (binary, midpoint edges)
            switch (idx)
            {
            case 1:  DrawLine(Pixels, W, H, left, bottom, LineColor, Thickness); break;
            case 2:  DrawLine(Pixels, W, H, bottom, right, LineColor, Thickness); break;
            case 3:  DrawLine(Pixels, W, H, left, right, LineColor, Thickness); break;
            case 4:  DrawLine(Pixels, W, H, top, right, LineColor, Thickness); break;
            case 5:
                DrawLine(Pixels, W, H, top, left, LineColor, Thickness);
                DrawLine(Pixels, W, H, right, bottom, LineColor, Thickness);
                break;
            case 6:  DrawLine(Pixels, W, H, top, bottom, LineColor, Thickness); break;
            case 7:  DrawLine(Pixels, W, H, top, left, LineColor, Thickness); break;
            case 8:  DrawLine(Pixels, W, H, top, left, LineColor, Thickness); break;
            case 9:  DrawLine(Pixels, W, H, top, bottom, LineColor, Thickness); break;
            case 10:
                DrawLine(Pixels, W, H, top, right, LineColor, Thickness);
                DrawLine(Pixels, W, H, left, bottom, LineColor, Thickness);
                break;
            case 11: DrawLine(Pixels, W, H, top, right, LineColor, Thickness); break;
            case 12: DrawLine(Pixels, W, H, left, right, LineColor, Thickness); break;
            case 13: DrawLine(Pixels, W, H, bottom, right, LineColor, Thickness); break;
            case 14: DrawLine(Pixels, W, H, left, bottom, LineColor, Thickness); break;
            default: break;
            }
        }
    }
}

bool UMapTextureGenerator::WorldToMapUV(const ANoiseTerrainActor* Terrain, float WorldX, float WorldY, float& OutU, float& OutV) const
{
    if (!Terrain) return false;

    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;
    if (HalfW <= 0.f || HalfH <= 0.f) return false;

    const FVector World(WorldX, WorldY, 0.f);
    const FVector Local = Terrain->GetActorTransform().InverseTransformPosition(World);

    OutU = (Local.X + HalfW) / (2.f * HalfW);
    OutV = (Local.Y + HalfH) / (2.f * HalfH);

    OutU = FMath::Clamp(OutU, 0.f, 1.f);
    OutV = FMath::Clamp(OutV, 0.f, 1.f);
    return true;
}

UTexture2D* UMapTextureGenerator::GenerateMapTexture(ANoiseTerrainActor* Terrain, const FMapGenSettings& Settings)
{
    if (!Terrain) return nullptr;

    const int32 W = Settings.MapWidth;
    const int32 H = Settings.MapHeight;
    if (W <= 1 || H <= 1) return nullptr;

    const float HalfW = Terrain->NumQuadsX * Terrain->GridSpacing * 0.5f;
    const float HalfH = Terrain->NumQuadsY * Terrain->GridSpacing * 0.5f;

    const float WaterZ = Terrain->WaterZ;
    const float PeakZ = Settings.PeakZ;

    // 1) Base fill per pixel (water / mountain / peaks)
    TArray<FColor> Pixels;
    Pixels.SetNum(W * H);

    const FTransform TerrainT = Terrain->GetActorTransform();

    for (int32 py = 0; py < H; ++py)
    {
        const float v = (float)py / (float)(H - 1);
        const float LocalY = FMath::Lerp(-HalfH, +HalfH, v);

        for (int32 px = 0; px < W; ++px)
        {
            const float u = (float)px / (float)(W - 1);
            const float LocalX = FMath::Lerp(-HalfW, +HalfW, u);

            const FVector LocalPos(LocalX, LocalY, 0.f);
            const FVector WorldPos = TerrainT.TransformPosition(LocalPos);

            const float Z = Terrain->GetHeightAtWorldXY(WorldPos.X, WorldPos.Y, true);

            FColor C;
            if (Z < WaterZ)
            {
                C = Settings.WaterColor;
            }
            else if (Z < PeakZ)
            {
                C = Settings.MountainColor;
            }
            else
            {
                C = Settings.PeakColor;
            }

            Pixels[py * W + px] = C;
        }
    }

    // 2) Boolean field for marching squares (shoreline at WaterZ)
    const int32 CellsX = FMath::Max(1, Settings.CellsX);
    const int32 CellsY = FMath::Max(1, Settings.CellsY);
    const int32 FX = CellsX + 1;
    const int32 FY = CellsY + 1;

    TArray<uint8> ShoreField;
    ShoreField.SetNum(FX * FY);

    for (int32 sy = 0; sy < FY; ++sy)
    {
        const float v = (float)sy / (float)(FY - 1);
        const float LocalY = FMath::Lerp(-HalfH, +HalfH, v);

        for (int32 sx = 0; sx < FX; ++sx)
        {
            const float u = (float)sx / (float)(FX - 1);
            const float LocalX = FMath::Lerp(-HalfW, +HalfW, u);

            const FVector LocalPos(LocalX, LocalY, 0.f);
            const FVector WorldPos = TerrainT.TransformPosition(LocalPos);

            const float Z = Terrain->GetHeightAtWorldXY(WorldPos.X, WorldPos.Y, true);
            ShoreField[sy * FX + sx] = (Z >= WaterZ) ? 1 : 0;
        }
    }

    // 3) Draw shoreline contour
    RasterizeMarchingSquares(Pixels, W, H, ShoreField, FX, FY, Settings.ShorelineColor, Settings.LineThickness);

    // 4) Optional second contour: peak boundary at PeakZ
    if (Settings.bDrawPeakContour)
    {
        TArray<uint8> PeakField;
        PeakField.SetNum(FX * FY);

        for (int32 sy = 0; sy < FY; ++sy)
        {
            const float v = (float)sy / (float)(FY - 1);
            const float LocalY = FMath::Lerp(-HalfH, +HalfH, v);

            for (int32 sx = 0; sx < FX; ++sx)
            {
                const float u = (float)sx / (float)(FX - 1);
                const float LocalX = FMath::Lerp(-HalfW, +HalfW, u);

                const FVector LocalPos(LocalX, LocalY, 0.f);
                const FVector WorldPos = TerrainT.TransformPosition(LocalPos);

                const float Z = Terrain->GetHeightAtWorldXY(WorldPos.X, WorldPos.Y, true);
                PeakField[sy * FX + sx] = (Z >= PeakZ) ? 1 : 0;
            }
        }

        RasterizeMarchingSquares(Pixels, W, H, PeakField, FX, FY, Settings.PeakContourColor, Settings.LineThickness);
    }

    // 5) Upload to transient texture
    UTexture2D* Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
    if (!Tex) return nullptr;

    Tex->MipGenSettings = TMGS_NoMipmaps;
    Tex->CompressionSettings = TC_VectorDisplacementmap; // crisp, no color artifacts
    Tex->SRGB = true;
    Tex->NeverStream = true;

    // NOTE: CreateTransient already creates PlatformData in UE5; still check safety.
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
