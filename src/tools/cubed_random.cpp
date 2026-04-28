#include "Cubed/tools/cubed_random.hpp"

#include "Cubed/tools/log.hpp"

namespace Cubed {

Random::Random() {}

bool Random::random_bool(double probability) {
    std::bernoulli_distribution dist(probability);
    return dist(m_engine);
}

std::mt19937& Random::engine() { return m_engine; }

unsigned Random::seed() { return m_seed; }

void Random::init(unsigned seed) {
    m_seed = seed;
    m_engine.seed(seed);
}

} // namespace Cubed