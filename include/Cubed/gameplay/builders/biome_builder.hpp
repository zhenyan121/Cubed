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

protected:
    void build_bottom();
};
} // namespace Cubed