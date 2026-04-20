#pragma once
#include <glad/glad.h>

#include <string>

namespace Cubed {


class Shader {
public:
    Shader();
    Shader(const std::string& name, const std::string& v_shader_path, const std::string& f_shader_path);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& shader) noexcept;
    Shader& operator=(Shader&& shader) noexcept;

    void create(const std::string& name, const std::string& v_shader_path, const std::string& f_shader_path);
    std::size_t hash() const;
    GLuint loc(const std::string& loc) const;
    const std::string& name() const;
    void use() const;

private:
    GLuint m_program = 0;
    std::size_t m_hash = 0;
    std::string m_name = "-1";
    
};

}