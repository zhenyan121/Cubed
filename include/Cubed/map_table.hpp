#pragma once
#include <string>
#include <unordered_map>

namespace Cubed {

class MapTable {
private:
    static std::unordered_map<unsigned, std::string> id_to_name_map;
    static std::unordered_map<size_t, unsigned> name_to_id_map;
public:
    // please using reference
    static const std::string& get_name_from_id(unsigned id);
    static unsigned get_id_from_name(const std::string& name);
    static void init_map();

};

}