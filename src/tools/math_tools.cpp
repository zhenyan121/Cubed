#include <Cubed/tools/math_tools.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cmath>

#include <Cubed/config.hpp>
#include <Cubed/gameplay/chunk_status.hpp>
#include <Cubed/tools/perlin_noise.hpp>

namespace Math {
    void extract_frustum_planes(const glm::mat4& mvp_matrix, std::vector<glm::vec4>& planes) {
        if (planes.size() != 6) {
            planes.resize(6);
        }
        
        const float* m = glm::value_ptr(mvp_matrix);

        // left plane
        planes[0] = glm::vec4(m[3]  + m[0], m[7]  + m[4], m[11] + m[8],  m[15] + m[12]);
        // right plane
        planes[1] = glm::vec4(m[3]  - m[0], m[7]  - m[4], m[11] - m[8],  m[15] - m[12]);
        // bottom plane
        planes[2] = glm::vec4(m[3]  + m[1], m[7]  + m[5], m[11] + m[9],  m[15] + m[13]);
        // top plane
        planes[3] = glm::vec4(m[3]  - m[1], m[7]  - m[5], m[11] - m[9],  m[15] - m[13]);
        // near plane
        planes[4] = glm::vec4(m[3]  + m[2], m[7]  + m[6], m[11] + m[10], m[15] + m[14]);
        // far plane
        planes[5] = glm::vec4(m[3]  - m[2], m[7]  - m[6], m[11] - m[10], m[15] - m[14]);

        for (auto& p : planes) {
            p = glm::normalize(p);
        }
    }

    int get_interpolated_height(float world_x, float world_z, float biome_noise, float temp, float humid) {
        

        auto weight = [](float t, float h, float ct, float ch) -> float {
            float dt = t - ct;
            float dh = h - ch;
            float dist = std::sqrt(dt*dt + dh*dh);
            return std::max(0.0f, 0.5f - dist); 
        };

        float w_mountain = weight(temp, humid, 0.25f, 0.25f);
        float w_plain    = weight(temp, humid, 0.25f, 0.75f);
        float w_desert   = weight(temp, humid, 0.75f, 0.25f);
        float w_forest   = weight(temp, humid, 0.75f, 0.75f);
        // adjust transitions between chunks
        float pow_n = 8.0f; // the larger n is, the purer the biome
        w_mountain = std::pow(w_mountain, pow_n);
        w_plain    = std::pow(w_plain,    pow_n);
        w_desert   = std::pow(w_desert,   pow_n);
        w_forest   = std::pow(w_forest,   pow_n);

        float total = w_mountain + w_plain + w_desert + w_forest;
        w_mountain /= total; w_plain /= total; w_desert /= total; w_forest /= total;

        auto sample_height = [&](Biome b) -> float {
            auto range = get_biome_height_range(b);
            auto [f1, f2, f3] = get_noise_frequencies_for_biome(b);
            float n =
                1.00f * PerlinNoise::noise(world_x * f1, 0.5f, world_z * f1) +
                0.50f * PerlinNoise::noise(world_x * f2, 0.5f, world_z * f2) +
                0.25f * PerlinNoise::noise(world_x * f3, 0.5f, world_z * f3);
            n /= 1.75f;
            return range.base_y + n * range.amplitude;
        };

        float h = w_mountain * sample_height(Biome::MOUNTAIN)
                + w_plain    * sample_height(Biome::PLAIN)
                + w_desert   * sample_height(Biome::DESERT)
                + w_forest   * sample_height(Biome::FOREST);
        return static_cast<int>(h);
    }
}