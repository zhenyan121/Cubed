#include <Cubed/config.hpp>
#include <Cubed/map_table.hpp>
#include <Cubed/tools/cubed_assert.hpp>
#include <Cubed/tools/cubed_hash.hpp>

#include <array>

std::unordered_map<unsigned, std::string> MapTable::id_to_name_map;
std::unordered_map<size_t, unsigned> MapTable::name_to_id_map;

constexpr std::array<std::string_view, MAX_BLOCK_NUM> BLOCK_REISTER{
    "air",
    "grass_block",
    "dirt",
    "stone",
    "sand",
    "log",
    "leaf"
};


const std::string& MapTable::get_name_from_id(unsigned id) {
    auto it = id_to_name_map.find(id);
    CUBED_ASSERT_MSG(it != id_to_name_map.end(), "Id: " + std::to_string(id) + " is not exist");
    return it->second;
}
const unsigned MapTable::get_id_from_name(const std::string& name) {
    auto it = name_to_id_map.find(HASH::str(name));
    CUBED_ASSERT_MSG(it != name_to_id_map.end(), "Name " + name + " is not exist");
    return it->second;
}

void MapTable::init_map() {
    id_to_name_map.reserve(MAX_BLOCK_NUM);
    name_to_id_map.reserve(MAX_BLOCK_NUM);

    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        id_to_name_map[i] = BLOCK_REISTER[i];
        name_to_id_map[HASH::str(BLOCK_REISTER[i])] = i;
    }
    
}


