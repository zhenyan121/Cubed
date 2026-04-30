#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace Cubed {

class MapTable {
private:
    static inline std::unordered_map<unsigned, std::string> id_to_name_map;
    static inline std::unordered_map<size_t, unsigned> name_to_id_map;
    static inline std::vector<std::string> item_id_to_name;

public:
    // please using reference
    static std::string_view get_name_from_id(unsigned id);
    static unsigned get_id_from_name(const std::string& name);
    static std::string_view item_name(unsigned id);
    static const std::vector<std::string>& item_map();
    static void init_map();
};

} // namespace Cubed