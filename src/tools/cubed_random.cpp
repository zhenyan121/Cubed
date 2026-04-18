#include <Cubed/tools/cubed_random.hpp>

#include <Cubed/tools/log.hpp>

#include <atomic>

namespace Cubed {

unsigned Random::get_base_seed() {
    static unsigned base = [] {
        std::random_device rd;
        return rd();
    }();
    return base;
}

unsigned Random::get_thread_seed() {
    static std::atomic<unsigned> counter{0};
    thread_local static unsigned seed = get_base_seed() + counter.fetch_add(1);
    return seed;
}

Random::Random() {
    m_seed = get_thread_seed();
    Logger::info("Seed: {}", m_seed);
    m_engine.seed(m_seed);
}

Random& Random::get() {
    thread_local Random instance;
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