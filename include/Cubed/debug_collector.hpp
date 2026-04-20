#pragma once

#include <Cubed/ui/text.hpp>

#include <unordered_map>

namespace Cubed {


class DebugCollector {
public:
    static DebugCollector& get();
    DebugCollector();
    
    std::unordered_map<std::size_t, Text>& all_texts();

    Text& text(std::string_view name);
    void report(std::string_view name, std::string_view content);
    void init_text();

private:
    std::unordered_map<std::size_t, Text> m_texts;
};

}