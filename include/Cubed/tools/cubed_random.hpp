#pragma once
#include <random>
namespace Cubed {

class Random {
public:
    Random();

    static Random& get();

    bool random_bool(double probability);
    std::mt19937& engine();
    unsigned seed();

private:
    unsigned int m_seed = 0;
    std::mt19937 m_engine;
};


}
