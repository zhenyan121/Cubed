#include <Cubed/config.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/log.hpp>

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace std::string_view_literals;

namespace Cubed {

Config::Config() {
    load_or_create_config();
}

Config::~Config() {
    save_to_file();
}

Config& Config::get() {
    static Config instance;
    return instance;
}

toml::table& Config::table() {
    return m_tbl;
}

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
    fs::path config_path {CONGIF_PATH};
    if (!fs::is_regular_file(config_path)) {
        create_config();
    } else try {
        m_tbl = toml::parse_file(config_path.string());
    } catch (const toml::parse_error& err) {
        Logger::error("Load Config Error: \"{}\"", err.what());
        create_config();
    }
    
    Logger::info("Load Config File Success");

}

void Config::save_to_file() {
    fs::path config_path {CONGIF_PATH};
    std::ofstream file{config_path};
    file << m_tbl;
    Logger::info("Save File Success");
}


}