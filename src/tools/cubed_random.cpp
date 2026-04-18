#include <Cubed/tools/cubed_random.hpp>

#include <Cubed/tools/log.hpp>

namespace Cubed {

Random::Random() {
    std::random_device d;
    m_seed = d();
    Logger::info("Seed: {}", m_seed);
    m_engine.seed(m_seed);
}

Random& Random::get() {
    static Random instance;
    return instance;
}

bool Random::random_bool(double probability) {
    std::bernoulli_distribution dist(probability);
    return dist(m_engine);
}

std::mt19937& Random::engine() {
    return m_engine;
}

unsigned Random::seed() {
    return m_seed;
}



}