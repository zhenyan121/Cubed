#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Cubed {

using BlockType = uint8_t;

struct BlockTexture {
    std::string name;
    unsigned id;
    std::vector<GLuint> texture;
};

struct Block : public BlockTexture {};

struct BlockRenderData {
    std::vector<bool> draw_face;
    unsigned block_id;
    BlockRenderData() = default;
    BlockRenderData(const BlockRenderData&) = default;
    BlockRenderData& operator=(const BlockRenderData&) = default;
    BlockRenderData(BlockRenderData&& data)
        : draw_face(std::move(data.draw_face)), block_id(data.block_id) {}
    BlockRenderData& operator=(BlockRenderData&& data) {
        draw_face = std::move(data.draw_face);
        block_id = data.block_id;
        return *this;
    }
};

struct LookBlock {
    glm::ivec3 pos;
    glm::ivec3 normal;
};

struct BlockData {
    std::string name;
    BlockType id = 0;

    bool is_liquid = false;

    bool is_passable = false;
    bool is_cross_plane = false;

    BlockData(BlockType b_id, std::string_view b_name, bool liquid,
              bool passable, bool cross_plane)
        : name(b_name), id(b_id), is_liquid(liquid), is_passable(passable),
          is_cross_plane(cross_plane) {}
};

class BlockManager {

public:
    static const std::vector<BlockData>& datas();
    static void init();
    static unsigned sums();
    static const std::string& name_form_id(unsigned id);

    static bool is_cross_plane(unsigned id);

private:
    static inline std::vector<BlockData> m_datas;
    static inline bool is_init = false;
};

} // namespace Cubed