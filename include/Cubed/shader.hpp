#pragma once
#include <glad/glad.h>

#include <string>
class Shader {
public:
    Shader();
    Shader(const std::string& name, const std::string& v_shader_path, const std::string& f_shader_path);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& shader);
    Shader& operator=(Shader&& shader);

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