/**
 * All fully specified implementations of methods declared in soundslot_collection need to go into this file.
 */

#include "soundslot_collection.h"

template<>
ssize_t soundslot_collection<soundslot_collection_vector_t>::find_existing_soundslot_(const soundslot& slot) {
    // classic linear search
    for (size_t i = 0; i < soundslots_.size(); ++i) {
        const auto& existing_slot = soundslots_[i];

        if (check_soundslots_equal_(slot, existing_slot)) {
            return i;
        }
    }

    return -1;
}

template<>
ssize_t soundslot_collection<soundslot_collection_map_t>::find_existing_soundslot_(const soundslot& slot) {
    // classic linear search through the map's values
    for (const auto& pair : soundslots_) {
        const auto i = pair.first;
        const auto& existing_slot = pair.second;

        if (check_soundslots_equal_(slot, existing_slot)) {
            return i;
        }
    }

    return -1;
}

template<>
size_t soundslot_collection<soundslot_collection_vector_t>::insert_into_soundslots_(ssize_t index, const soundslot &slot) {
    if (index >= 0) {
        throw std::logic_error{"Specific indices not supported for vector type collections"};
    }

    soundslots_.emplace_back(slot);
    return soundslots_.size() - 1;
}

template<>
size_t soundslot_collection<soundslot_collection_map_t>::insert_into_soundslots_(ssize_t index, const soundslot &slot) {
    if (index < 0) {
        throw std::logic_error{"Must specify index for map type collections"};
    }

    soundslots_[index] = slot;
    return index;
}

template<>
size_t soundslot_collection<soundslot_collection_vector_t>::append(const std::string& name, int volume, int max_radius, int min_radius, int num_slots) {
    return insert_or_append_(name, volume, max_radius, min_radius, num_slots, -1);
}

template<>
size_t soundslot_collection<soundslot_collection_map_t>::insert(size_t index, const std::string& name, int volume, int max_radius, int min_radius, int num_slots) {
    return insert_or_append_(name, volume, max_radius, min_radius, num_slots, index);
}

template<>
bool soundslot_collection<soundslot_collection_map_t>::contains(size_t i) const {
    return soundslots_.find(i) != soundslots_.end();
}

template<>
bool soundslot_collection<soundslot_collection_vector_t>::contains(size_t i) const {
    return i < soundslots_.size();
}

template<>
size_t soundslot_collection<soundslot_collection_vector_t>::size() const {
    return soundslots_.size();
}
