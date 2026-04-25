#pragma once
#include <toml++/toml.hpp>

#include <Cubed/tools/cubed_assert.hpp>

namespace Cubed {


template <typename T>
concept TomlValueType = 
    std::same_as<T, int> ||
    std::same_as<T, bool> ||
    std::same_as<T, double> ||
    std::same_as<T, const char*> ||
    std::same_as<T, toml::date> ||
    std::same_as<T, toml::time> ||
    std::same_as<T, toml::date_time> ||
    std::same_as<T, std::string> 
    ;

class Config {
public:
    Config();
    ~Config();

    static Config& get();

    toml::table& table();

    void load_or_create_config();
    void save_to_file();

    template <TomlValueType T>
    T get(std::string_view key) const{
        size_t cur = 0;
        auto pos = key.find('.');
        const toml::table* table = &m_tbl;
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
        auto opt = (*table)[n_key].value<T>();
        if (opt){
            return *opt;
        } else {
            Logger::error("Can't find key {}", n_key);
            ASSERT(false);
            std::abort();
        }
    }
    template <typename T>
    void set(std::string_view key, T&& val) {
        if constexpr (!TomlValueType<std::decay_t<T>>) {
            static_assert(false, "Type Not Support");
        } 
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
                auto [it, inserted] = table->insert_or_assign(s, toml::table{});
                table = it->second.as_table();
            }
            
        }
        std::string_view n_key = key.substr(cur);
        if (n_key.empty()) {
            Logger::error("Trailing dot in path '{}'", key);
            ASSERT(false);
            std::abort();
        }
        table->insert_or_assign(n_key, std::forward<T>(val));
        
    }
    template <typename T>
    void set_and_save(std::string_view key, T&& val)  {
        set(key, std::forward(val));
        save_to_file();
    }
    toml::node_view<toml::node> val_view(std::string_view key);

private:
    
    toml::table m_tbl;
    constexpr static inline std::string_view CONGIF_PATH = ASSETS_PATH"config.toml";
    void create_config();

};

}
