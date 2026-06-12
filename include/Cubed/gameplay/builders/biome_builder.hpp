#pragma once

namespace Cubed {
class ChunkGenerator;
class BiomeBuilder {
public:
    BiomeBuilder() = default;
    virtual ~BiomeBuilder() = default;
    virtual ChunkGenerator& get_chunk_generator() = 0;
    virtual void build_biome() = 0;
    virtual void build_vegetation() = 0;
    void ocean_water_build();

protected:
    void build_bottom();
    void place_grass();
};
} // namespace Cubed