#pragma once

#include "Cubed/gameplay/builders/biome_builder.hpp"
namespace Cubed {

class ChunkGenerator;

class DesertBuilder : public BiomeBuilder {
public:
    DesertBuilder(ChunkGenerator& chunk_generator);
    void build_biome() override;
    ChunkGenerator& get_chunk_generator() override;
    void build_vegetation() override;

private:
    ChunkGenerator& m_chunk_generator;

    void build_blocks();
};

} // namespace Cubed
