#include "Cubed/map_table.hpp"

#include "Cubed/gameplay/block.hpp"
#include "Cubed/tools/cubed_assert.hpp"
#include "Cubed/tools/cubed_hash.hpp"

namespace Cubed {

std::string_view MapTable::get_name_from_id(unsigned id) {
    auto it = id_to_name_map.find(id);
    ASSERT_MSG(it != id_to_name_map.end(),
               "Id: " + std::to_string(id) + " is not exist");
    return it->second;
}

unsigned MapTable::get_id_from_name(const std::string& name) {
    auto it = name_to_id_map.find(HASH::str(name));
    ASSERT_MSG(it != name_to_id_map.end(), "Name " + name + " is not exist");
    return it->second;
}

std::string_view MapTable::item_name(unsigned id) {
    ASSERT_MSG(id < item_id_to_name.size(), "ID is invalid");
    return item_id_to_name[id];
}

const std::vector<std::string>& MapTable::item_map() { return item_id_to_name; }

void MapTable::init_map() {
    id_to_name_map.reserve(MAX_BLOCK_NUM);
    name_to_id_map.reserve(MAX_BLOCK_NUM);

    for (int i = 0; i < MAX_BLOCK_NUM; i++) {
        id_to_name_map[i] = BLOCK_REISTER[i];
        name_to_id_map[HASH::str(BLOCK_REISTER[i])] = i;
    }
    for (auto s : BLOCK_REISTER) {
        item_id_to_name.emplace_back(s);
    }
}

} // namespace Cubed
