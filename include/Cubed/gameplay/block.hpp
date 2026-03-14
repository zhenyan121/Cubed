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
    glm::vec3 pos;
    std::vector<bool> draw_face;
    unsigned block_id;
};

struct LookBlock {
    glm::ivec3 pos;
    glm::ivec3 normal;
};