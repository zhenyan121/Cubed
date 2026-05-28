#include "Cubed/gameplay/block.hpp"

#include "Cubed/config.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"

#include <filesystem>
#include <toml++/toml.hpp>

namespace fs = std::filesystem;

using namespace std::string_literals;

namespace {
std::string block_data_dir = ASSETS_PATH + "data/block"s;

template <Cubed::TomlValueType T>
std::optional<T> safe_get_value(const toml::table& table, std::string_view key,
                                const T& default_value) {
    auto value = table[key].value<T>();
    if (value == std::nullopt) {
        Cubed::Logger::warn("Key {} Is Not Find, Wiil Set the Default Value {}",
                            key, default_value);
        value = default_value;
    }
    return value;
}

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
const std::string& BlockManager::name_form_id(unsigned id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].name;
    }
    return m_datas[id].name;
}

bool BlockManager::is_cross_plane(unsigned id) {
    if (id >= sums()) {
        Logger::error("Id {}, is Over The Max Id", id, sums() - 1);
        return m_datas[0].is_cross_plane;
    }
    return m_datas[id].is_cross_plane;
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
            block = toml::parse_file(entry.path().c_str());
        } catch (const toml::parse_error& err) {
            Logger::error("Load Block Data {} Fail, Parser Error {}",
                          entry.path().c_str(), err.what());
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

        m_datas.emplace_back(*id, *name, *is_liquid, *is_passable,
                             *is_cross_plane);
    }
    std::sort(
        m_datas.begin(), m_datas.end(),
        [](const BlockData& a, const BlockData& b) { return a.id < b.id; });
    is_init = true;
}

} // namespace Cubed