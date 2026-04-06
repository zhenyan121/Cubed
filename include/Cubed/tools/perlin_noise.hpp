#pragma once

#include <vector>

class PerlinNoise {
public:
    static void init(unsigned int seed);
    static float noise(float x, float y, float z);
private:
    static inline bool is_init = false;
    static inline std::vector<int> p;

    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y, float z);
};