#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>
#include <string>
#include <vector>

#include <Cubed/config.hpp>
#include <Cubed/tools/cubed_assert.hpp>

namespace Cubed {


struct BlockTexture {
    std::string name;
    unsigned id;
    std::vector<GLuint> texture;
};

struct Block : public BlockTexture{

};

struct BlockRenderData {
    std::vector<bool> draw_face;
    unsigned block_id;
    BlockRenderData() = default;
    BlockRenderData(const BlockRenderData&) = default;
    BlockRenderData& operator=(const BlockRenderData&) = default;
    BlockRenderData(BlockRenderData&& data) :
        draw_face(std::move(data.draw_face)),
        block_id(data.block_id)
    {
        
    }
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

constexpr std::array<std::string_view, MAX_BLOCK_NUM> BLOCK_REISTER{
    "air",
    "grass_block",
    "dirt",
    "stone",
    "sand",
    "log",
    "leaf"
};

const std::array<bool, MAX_BLOCK_NUM> TRANSPARENT_MAP {
    true,
    false,
    false,
    false,
    false,
    false,
    true
};

inline bool is_in_transparent_map(unsigned id) {
    ASSERT_MSG(id < MAX_BLOCK_NUM, "ID is invaild");
    return TRANSPARENT_MAP[id];
};


}