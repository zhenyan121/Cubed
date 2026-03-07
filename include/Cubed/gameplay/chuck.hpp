#pragma once

#include <cstdint>

#include <Cubed/config.hpp>
#include <Cubed/gameplay/chuck_status.hpp>
#include <Cubed/gameplay/block.hpp>



class Chuck {
private:

    // the index is a array of block id
    std::vector<uint8_t> m_blocks;

public:
    Chuck();
    ~Chuck();
    const std::vector<uint8_t>& get_chuck_blocks() const;    
    void init_chuck();
    static int get_index(int x, int y, int z);
};