#include <Cubed/tools/perlin_noise.hpp>

#include <Cubed/config.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_random.hpp>
#include <algorithm>
#include <numeric>
#include <random>

void PerlinNoise::init() {
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    auto seed = std::random_device{}();
    Logger::info("Init Perlin Noise With Seed {}", seed);
    std::shuffle(p.begin(), p.end(), std::mt19937(seed));

    p.insert(p.end(), p.begin(), p.end());
    is_init = true;
}

float PerlinNoise::noise(float x, float y, float z) {
    CUBED_ASSERT_MSG(is_init, "The PerlinNoise don't init!");
    int ix = static_cast<int>(std::floor(x)) & 255;
    int iy = static_cast<int>(std::floor(y)) & 255;
    int iz = static_cast<int>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    int a = p[ix] + iy;
    int aa = p[a] + iz;
    int ab = p[a + 1] + iz;
    int b = p[ix + 1] + iy;
    int ba = p[b] + iz;
    int bb = p[b + 1] + iz;

    float res = lerp (w,
        lerp (v,
            lerp(u, grad(p[aa], x, y, z), grad(p[ba], x - 1, y, z)),
            lerp(u, grad(p[ab], x, y - 1, z), grad(p[bb], x - 1, y - 1, z))
        ),
        lerp(v,
            lerp(u, grad(p[aa + 1], x, y, z - 1), grad(p[ba + 1], x - 1, y, z - 1)),
            lerp(u, grad(p[ab + 1], x, y - 1, z - 1), grad(p[bb + 1 ], x - 1, y - 1, z - 1))
        )
        
    );
    return (res + 1.0f) / 2.0f;
}

float PerlinNoise::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y, float z) {
    int h = hash & 15;

    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;

    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}