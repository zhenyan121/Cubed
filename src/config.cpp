#include "Cubed/config.hpp"

#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/log.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace std::string_view_literals;

namespace Cubed {

Config::Config() { load_or_create_config(); }

Config::~Config() { save_to_file(); }

Config& Config::get() {
    static Config instance;
    return instance;
}

toml::table& Config::table() { return m_tbl; }

void Config::create_config() {
    static constexpr auto SOURCE = R"(
        version = "0.0.1"

        [window]
        width = 800
        height = 600
        fullscreen = false
        V-Sync = true

        [player]
        fov = 70.0 
        mouse_sensitivity = 0.15

        [world]
        rendering_distance = 24
        
        [devpanel]
        theme = 0       # 0 is Dark Theme, 1 is Light Theme

        [texture]
        aniso = 1       # i is the minimun value, indicating off 

    )"sv;

    try {
        m_tbl = toml::parse(SOURCE);
    } catch (const toml::parse_error& err) {
        Logger::error("Load Config Error {}", err.what());
        ASSERT(false);
        std::abort();
    }
    Logger::info("Create New Config File Success");
}

void Config::load_or_create_config() {
    fs::path config_path{CONGIF_PATH};
    if (!fs::is_regular_file(config_path)) {
        create_config();
    } else
        try {
            m_tbl = toml::parse_file(config_path.string());
        } catch (const toml::parse_error& err) {
            Logger::error("Load Config Error: \"{}\"", err.what());
            create_config();
        }

    Logger::info("Load Config File Success");
}

void Config::save_to_file() {
    fs::path config_path{CONGIF_PATH};
    std::ofstream file{config_path};
    file << m_tbl;
    Logger::info("Save File Success");
}

toml::node_view<toml::node> Config::val_view(std::string_view key) {
    size_t cur = 0;
    auto pos = key.find('.');
    toml::table* table = &m_tbl;
    while (pos != std::string_view::npos) {
        std::string_view s = key.substr(cur, pos - cur);
        if (s.empty()) {
            Logger::error("Empty key/table name in path '{}'", key);
            ASSERT(false);
            std::abort();
        }
        cur = pos + 1;
        pos = key.find('.', cur);
        if (auto* next = (*table)[s].as_table()) {
            table = next;
        } else {
            Logger::error("Can't find table {}", s);
            ASSERT(false);
            std::abort();
        }
    }
    std::string_view n_key = key.substr(cur);
    if (n_key.empty()) {
        Logger::error("Trailing dot in path '{}'", key);
        ASSERT(false);
        std::abort();
    }
    auto view = (*table)[n_key];
    if (!view) {
        Logger::error("The view is null");
        ASSERT(false);
        std::abort();
    }
    return view;
}

} // namespace Cubed