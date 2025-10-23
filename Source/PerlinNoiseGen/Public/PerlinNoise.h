#pragma once
#include <array>
#include <algorithm>
#include <random>
#include <numeric>   // <-- add this line for std::iota

/**
 * Minimal, seedable 2D Perlin noise + fBm.
 * Range of base Noise2D is approximately [-1, 1].
 */
class FPerlinNoise
{
public:
    explicit FPerlinNoise(int32_t Seed = 1337)
    {
        reseed(Seed);
    }

    void reseed(int32_t Seed)
    {
        // Build permutation with seed
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);

        std::mt19937 rng(static_cast<uint32_t>(Seed));
        std::shuffle(p.begin(), p.end(), rng);

        // Duplicate to avoid overflow on lookups
        p.insert(p.end(), p.begin(), p.end());
    }

    // Base 2D Perlin in [-1,1]
    float Noise2D(float x, float y) const
    {
        // Find unit grid cell containing point
        int X = static_cast<int>(floorf(x)) & 255;
        int Y = static_cast<int>(floorf(y)) & 255;

        // Relative x, y within cell
        float xf = x - floorf(x);
        float yf = y - floorf(y);

        // Fade curves
        float u = fade(xf);
        float v = fade(yf);

        // Hash coordinates of the 4 corners
        int aa = p[p[X] + Y];
        int ab = p[p[X] + Y + 1];
        int ba = p[p[X + 1] + Y];
        int bb = p[p[X + 1] + Y + 1];

        // Add blended results from 4 corners
        float x1 = lerp(grad(aa, xf, yf),
            grad(ba, xf - 1, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1),
            grad(bb, xf - 1, yf - 1), u);
        float val = lerp(x1, x2, v);

        // val is roughly in [-1,1]
        return val;
    }

    // Fractal Brownian Motion (octaves of Perlin)
    float FBm2D(float x, float y, int Octaves, float Lacunarity, float Persistence) const
    {
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float sum = 0.0f;
        float ampSum = 0.0f;

        for (int i = 0; i < Octaves; ++i)
        {
            sum += amplitude * Noise2D(x * frequency, y * frequency);
            ampSum += amplitude;
            amplitude *= Persistence;
            frequency *= Lacunarity;
        }

        // Normalize to [-1,1] (roughly)
        if (ampSum > 0.0f) sum /= ampSum;
        return sum;
    }

private:
    std::vector<int> p;

    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float lerp(float a, float b, float t) { return a + t * (b - a); }

    static float grad(int hash, float x, float y)
    {
        // 8 gradients
        switch (hash & 7)
        {
        case 0:  return  x + y;
        case 1:  return  x - y;
        case 2:  return -x + y;
        case 3:  return -x - y;
        case 4:  return  x;
        case 5:  return -x;
        case 6:  return  y;
        default: return -y;
        }
    }
};
