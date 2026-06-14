#include "Cubed/tools/cubed_random.hpp"

namespace Cubed {

Random::Random() {}
Random::Random(unsigned seed) { init(seed); }
bool Random::random_bool(double probability) {
    if (probability <= 0.0)
        return false;
    if (probability >= 1.0)
        return true;

    const double MAX_VAL = 4294967295.0;
    unsigned threshold = static_cast<unsigned>(probability * MAX_VAL);

    return m_engine() <= threshold;
}

std::mt19937& Random::engine() { return m_engine; }

unsigned Random::seed() { return m_seed; }

void Random::init(unsigned seed) {
    m_seed = seed;
    m_engine.seed(seed);
}
int Random::random_int(int min, int max) {
    unsigned range = static_cast<unsigned>(max - min) + 1;

    const unsigned LIMIT =
        (std::numeric_limits<unsigned>::max() / range) * range;
    unsigned r;
    do {
        r = m_engine();
    } while (r >= LIMIT);

    return min + static_cast<int>(r % range);
}
float Random::random_float(float min, float max) {

    unsigned r = m_engine() >> 8;
    float t = static_cast<float>(r) * (1.0f / 16777216.0f);
    float result = min + t * (max - min);

    return result;
}
} // namespace Cubed