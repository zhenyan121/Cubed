#include "Cubed/tools/cubed_random.hpp"

namespace Cubed {

Random::Random() {}
Random::Random(unsigned seed) { init(seed); }
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
int Random::random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_engine);
}
float Random::random_float(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_engine);
}

} // namespace Cubed