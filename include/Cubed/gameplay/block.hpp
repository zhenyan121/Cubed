#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>



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