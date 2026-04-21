#include <Cubed/shader.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_hash.hpp>
#include <Cubed/tools/log.hpp>
#include <Cubed/tools/shader_tools.hpp>

namespace Cubed {


Shader::Shader() {

}

Shader::Shader(const std::string& name, const std::string& v_shader_path, const std::string& f_shader_path) {
    m_program = Tools::create_shader_program(v_shader_path, f_shader_path);
    m_name = name;
    m_hash = HASH::str(name);
}

Shader::Shader(Shader&& shader) noexcept:
    m_program(shader.m_program),
    m_hash(shader.m_hash),
    m_name(std::move(shader.m_name))
{
    shader.m_hash = 0;
    shader.m_program = 0;
}

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

Shader& Shader::operator=(Shader&& shader) noexcept{
    m_hash = shader.m_hash;
    m_name = std::move(shader.m_name);
    m_program = shader.m_program;
    shader.m_hash = 0;
    shader.m_program = 0;
    return *this;
}

void Shader::create(const std::string& name, const std::string& v_shader_path, const std::string& f_shader_path) {
    if (!m_program) {
        Logger::warn("Shader has already created !");
        return;
    }
    m_program = Tools::create_shader_program(v_shader_path, f_shader_path);
    m_name = name;
    m_hash = HASH::str(name);
}

std::size_t Shader::hash() const {
    if (!m_hash) {
        Logger::warn("Shader has already created !");
        return 0;
    }
    return m_hash;
}

GLuint Shader::loc(const std::string& loc) const {
    ASSERT_MSG(m_program != 0, "Shader program not created");
    GLint pos = glGetUniformLocation(m_program, loc.c_str());
    if (pos == -1) {
        Logger::info("Shader name {}, loc name {}, pos {}", m_name, loc, pos);
        ASSERT_MSG(pos == -1, "Can't find UniformLocation");
    }
    return static_cast<GLuint>(pos);
}

const std::string& Shader::name() const {
    if (m_name == "-1") {
        Logger::warn("Shader has already created !");
        
    }
    return m_name;
}

void Shader::use() const{
    ASSERT_MSG(m_program, "Shader don't create !");
    glUseProgram(m_program);
}

}