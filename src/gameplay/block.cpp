#include "Cubed/gameplay/block.hpp"

#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"
#include "Cubed/tools/toml.utils.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace std::string_literals;
using namespace Cubed::TOML;
namespace {
std::string block_data_dir = ASSETS_PATH + "data/block"s;

} // namespace

namespace Cubed {

const std::vector<BlockData>& BlockManager::datas() {
    ASSERT(is_init);
    return m_datas;
}

unsigned BlockManager::sums() {
    ASSERT(is_init);
    return m_datas.size();
}
unsigned BlockManager::cross_plane_sum() {
    ASSERT(is_init);
    return m_cross_plane_map.size();
}

const std::string& BlockManager::name_form_id(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].name;
    }
    return m_datas[id].name;
}

bool BlockManager::is_gas(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_gas;
    }
    return m_datas[id].is_gas;
}
bool BlockManager::is_liquid(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_liquid;
    }
    return m_datas[id].is_liquid;
}

bool BlockManager::is_cross_plane(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_cross_plane;
    }
    return m_datas[id].is_cross_plane;
}

bool BlockManager::is_transparent(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_transparent;
    }
    return m_datas[id].is_transparent;
}
bool BlockManager::is_passable(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_passable;
    }
    return m_datas[id].is_passable;
}

bool BlockManager::is_discard(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_discard;
    }
    return m_datas[id].is_discard;
}
bool BlockManager::is_blend(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_blend;
    }
    return m_datas[id].is_blend;
}
bool BlockManager::is_transitional(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_transitional;
    }
    return m_datas[id].is_transitional;
}

float BlockManager::roughness(BlockType id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].roughness;
    }
    return m_datas[id].roughness;
}

void BlockManager::init() {
    fs::path data_path{block_data_dir};

    for (auto entry : fs::recursive_directory_iterator(data_path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().filename() == "template.toml") {
            continue;
        }
        toml::table block;
        try {
            block = toml::parse_file(entry.path().string());
        } catch (const toml::parse_error& err) {
            Logger::error("Load Block Data {} Fail, Parser Error {}",
                          entry.path().string(), err.what());
            ASSERT(false);
        }
        auto id = block["id"].value<int>();
        if (id == std::nullopt) {
            Logger::error("Very Serious Error, Block Id Not Find !!!, Please "
                          "Check The Block Data Integrity");
            std::abort();
        }
        auto name = block["name"].value<std::string>();
        if (name == std::nullopt) {
            Logger::error("Very Serious Error, Block Name Not Find !!!, Please "
                          "Check The Block Data Integrity");
            std::abort();
        }
        auto is_liquid = safe_get_value(block, "is_liquid", false);
        auto is_passable = safe_get_value(block, "is_passable", false);
        auto is_cross_plane = safe_get_value(block, "is_cross_plane", false);
        auto is_transparent = safe_get_value(block, "is_transparent", false);
        auto is_gas = safe_get_value(block, "is_gas", false);
        auto is_discard = safe_get_value(block, "is_discard", false);
        auto is_blend = safe_get_value(block, "is_blend", false);
        auto is_transitional = safe_get_value(block, "is_transitional", false);
        auto roughness = safe_get_value(block, "roughness", 1.0);
        m_datas.emplace_back(*id, *name, *is_liquid, *is_passable,
                             *is_cross_plane, *is_transparent, *is_gas,
                             *is_discard, *is_blend, *is_transitional,
                             static_cast<float>(*roughness));
    }
    std::sort(
        m_datas.begin(), m_datas.end(),
        [](const BlockData& a, const BlockData& b) { return a.id < b.id; });

    set_up_cross_plane_map();
    is_init = true;
}

BlockType BlockManager::cross_plane_index(BlockType id) {
    auto it = m_cross_plane_map.find(id);
    if (it == m_cross_plane_map.end()) {
        Logger::error("Can't Find Cross Plane Id {}", id);
        ASSERT(false);
        throw std::out_of_range{"Can't Find Cross Plane Id" +
                                std::to_string(id)};
    }
    return it->second;
}

void BlockManager::set_up_cross_plane_map() {
    unsigned cur_id = 0;
    for (const auto& data : m_datas) {
        if (data.is_cross_plane) {
            m_cross_plane_map[data.id] = cur_id;
            cur_id++;
        }
    }
}

} // namespace Cubed