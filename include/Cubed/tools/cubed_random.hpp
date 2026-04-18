#pragma once
#include <random>
namespace Cubed {

class Random {
public:
    static unsigned get_base_seed();
    static unsigned get_thread_seed();
    static Random& get();

    bool random_bool(double probability);
    std::mt19937& engine();
    unsigned seed();

private:
    Random();
    unsigned int m_seed = 0;
    std::mt19937 m_engine;
};


}
