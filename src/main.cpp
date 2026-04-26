#include <Cubed/app.hpp>

int main(int argc, char** argv) {
    
    static_assert(sizeof(int) == sizeof(int32_t));
    static_assert(sizeof(unsigned int) == sizeof(uint32_t));

    return Cubed::App::start_cubed_application(argc, argv);
}


