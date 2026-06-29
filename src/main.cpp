#include "Cubed/app.hpp"

#ifdef _WIN32

extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif

int main(int argc, char** argv) {

    static_assert(sizeof(int) == sizeof(int32_t));
    static_assert(sizeof(unsigned int) == sizeof(uint32_t));

    return Cubed::App::start_cubed_application(argc, argv);
}
