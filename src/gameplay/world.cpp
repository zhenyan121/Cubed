#include <Cubed/gameplay/world.hpp>
#include <Cubed/tools/cubed_assert.hpp>
World::World() {
    
}

World::~World() {

}
static int chuck_x, chuck_z;
const BlockRenderData& World::get_block_render_data(int world_x, int world_y ,int world_z) {
    
    chuck_x = world_x / CHUCK_SIZE;
    chuck_z = world_z / CHUCK_SIZE;

    auto it = m_chucks.find((ChuckPos){chuck_x, chuck_z});
    CUBED_ASSERT_MSG(it != m_chucks.end(), "Chuck not find");
    
    const auto& chuck_blocks = it->second.get_chuck_blocks();
    int x, y, z;
    y = world_y;
    x = world_x - chuck_x * CHUCK_SIZE;
    z = world_z - chuck_z * CHUCK_SIZE;
    // block id
    m_block_render_data.block_id = chuck_blocks[Chuck::get_index(x, y, z)];
    // draw_face
    m_block_render_data.draw_face.assign(6, true);
    if (x > 0 ) {
        if (x > 0 && chuck_blocks[Chuck::get_index(x - 1, y, z)]) {
            m_block_render_data.draw_face[3] = false;
        }   
    }
    if (x < CHUCK_SIZE - 1) {
        if (x < DISTANCE * CHUCK_SIZE - 1 && chuck_blocks[Chuck::get_index(x + 1, y, z)]) {
            m_block_render_data.draw_face[1] = false;
        }
    }
    if (z > 0 ) {    
        if (z > 0 && chuck_blocks[Chuck::get_index(x, y, z - 1)]) {
            m_block_render_data.draw_face[2] = false;
        }
    }
    if (z < CHUCK_SIZE - 1) {
        if (z < DISTANCE * CHUCK_SIZE - 1 && chuck_blocks[Chuck::get_index(x, y, z + 1)]) {
            m_block_render_data.draw_face[0] = false;
        }
    }
    if (y > 0 ) {
        if (y > 0 && chuck_blocks[Chuck::get_index(x, y - 1, z)]) {
            m_block_render_data.draw_face[5] = false;
        }
    }
    if (y < CHUCK_SIZE - 1) {
        if (y < CHUCK_SIZE - 1 && chuck_blocks[Chuck::get_index(x, y + 1, z)]) {
            m_block_render_data.draw_face[4] = false;
        }
    }
    

    return m_block_render_data;
}

void World::init_world() {
    for (int s = 0; s < DISTANCE; s++) {
        for (int t = 0; t < DISTANCE; t++) {
            ChuckPos pos{s, t};
            Chuck chuck;
            m_chucks[pos] = chuck; 
        }
    }
    
}

void World::render() {
    
}