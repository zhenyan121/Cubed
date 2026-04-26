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

private:
    unsigned int m_seed = 0;
    std::mt19937 m_engine;
};


}
