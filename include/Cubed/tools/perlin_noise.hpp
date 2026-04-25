#pragma once
#include <atomic>
#include <vector>

namespace Cubed {


class PerlinNoise {
public:
    static void init();
    static float noise(float x, float y, float z);
    static void reload();
    static const unsigned& seed();
    static void seed(unsigned seed);
private:
    static inline std::atomic<bool> is_init = false;
    static inline std::vector<int> p;
    static inline unsigned m_seed = 0;
    static inline bool is_seed_change = false;
    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y, float z);
    
};

}