#include <Cubed/debug_collector.hpp>

#include <Cubed/tools/cubed_hash.hpp>

DebugCollector::DebugCollector() {

}

DebugCollector& DebugCollector::get() {
    static DebugCollector instance;
    return instance;
}

void DebugCollector::init_text() {
    Text version_text("version");
    Text fps_text("fps");
    Text player_pos_text("player_pos");
    version_text
        .position(0.0f, 100.0f)
        .scale(0.8f)
        .color(Color::WHITE)
        .text("Version: v0.0.1-Debug");
    fps_text
        .position(0.0f, 50.0f)
        .text("FPS: 0");
    player_pos_text
        .position(0.0f, 150.0f)
        .scale(0.8f)
        .text("x: 0.00 y: 0.00 z: 0.00");

    m_texts.insert({version_text.uuid(), std::move(version_text)});
    m_texts.insert({fps_text.uuid(), std::move(fps_text)});
    m_texts.insert({player_pos_text.uuid(), std::move(player_pos_text)});

}

std::unordered_map<std::size_t, Text>& DebugCollector::all_texts() {
    return m_texts;
}

Text& DebugCollector::text(std::string_view name) {
    std::size_t id  = HASH::str(name);
    auto it = m_texts.find(id);
    CUBED_ASSERT_MSG(it != m_texts.end(), "Can't Find Text");
    return it->second;
}

void DebugCollector::report(std::string_view name, std::string_view content) {
    auto& t = text(name);
    t.text(content);
}