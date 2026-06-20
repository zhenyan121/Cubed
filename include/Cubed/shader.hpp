#pragma once
#include "glm/ext/vector_float3.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <glad/glad.h>
#include <string>
#include <unordered_map>
namespace Cubed {

class Shader {
public:
    Shader();
    Shader(const std::string& name, const std::string& v_shader_path,
           const std::string& f_shader_path);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& shader) noexcept;
    Shader& operator=(Shader&& shader) noexcept;

    void create(const std::string& name, const std::string& v_shader_path,
                const std::string& f_shader_path);
    std::size_t hash() const;
    GLuint loc(const std::string& loc) const;
    const std::string& name() const;
    void use() const;
    template <typename> struct always_false : std::false_type {}; // NOLINT
    template <typename T>
    void set_loc(const std::string& location, T&& value) const {
        using std::is_same_v;
        using dT = std::decay_t<T>;
        if constexpr (is_same_v<dT, int> || is_same_v<dT, bool>) {
            glUniform1i(loc(location), value);
        } else if constexpr (is_same_v<dT, float>) {
            glUniform1f(loc(location), value);
        } else if constexpr (is_same_v<dT, double>) {
            glUniform1f(loc(location), static_cast<float>(value));
        } else if constexpr (is_same_v<dT, glm::vec3>) {
            glUniform3fv(loc(location), 1, glm::value_ptr(value));
        } else if constexpr (is_same_v<dT, glm::mat4>) {
            glUniformMatrix4fv(loc(location), 1, GL_FALSE,
                               glm::value_ptr(value));
        } else {
            static_assert(always_false<dT>::value, "Unknown Type");
        }
    };

private:
    GLuint m_program = 0;
    std::size_t m_hash = 0;
    std::string m_name = "-1";
    mutable std::unordered_map<std::string, GLint> m_uniform_cache;
};

} // namespace Cubed