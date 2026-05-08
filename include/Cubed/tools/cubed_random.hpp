#pragma once
#include <random>
namespace Cubed {

class Random {
public:
    Random();

    bool random_bool(double probability);
    std::mt19937& engine();
    unsigned seed();

    void init(unsigned seed);
    int random_int(int min, int max);
    float random_float(float min, float max);

private:
    unsigned int m_seed = 0;
    std::mt19937 m_engine;
};

} // namespace Cubed
