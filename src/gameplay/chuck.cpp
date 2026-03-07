#include <Cubed/gameplay/chuck.hpp>

Chuck::Chuck() {
    init_chuck();
}

Chuck::~Chuck() {

}

const std::vector<uint8_t>& Chuck::get_chuck_blocks() const{
    return m_blocks;
}

void Chuck::init_chuck() {
    m_blocks.assign(CHUCK_SIZE * CHUCK_SIZE * CHUCK_SIZE, 0);
    for (int x = 0; x < CHUCK_SIZE; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < CHUCK_SIZE; z++) {
                m_blocks[get_index(x, y, z)] = 1;
            }
        }
    }
}

int Chuck::get_index(int x, int y, int z) {
    return x * CHUCK_SIZE * CHUCK_SIZE + y * CHUCK_SIZE + z;
}
