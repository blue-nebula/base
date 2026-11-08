#pragma once

// system/STL headers
#include <stdexcept>
#include <map>

// local headers
#include "SDL.h"
#include "soundsample.h"
#include "soundslot.h"
#include "tools.h"

// forward declarations
extern hashnameset<soundsample> soundsamples;

// types used as internal containers in soundslot collections
typedef std::map<size_t, soundslot> soundslot_collection_map_t;
// TODO: use std::vector
typedef vector<soundslot> soundslot_collection_vector_t;

// forward declarations
extern Mix_Chunk* loadwav(const char *name);

/**
 * soundslots collection wrapper class. Follows "template method" pattern.
 * While game sounds are best managed in a map, map sounds need to be managed in a contiguous collection (no pun intended).
 * @tparam T collection type
 */
template<typename T>
class soundslot_collection {
private:
    // internal collection of soundslots
    T soundslots_;

    /**
     * Compare two soundslots for equality.
     */
    static bool check_soundslots_equal_(const soundslot& first, const soundslot& second);

    /**
     * Find existing soundslot in collection. Implemented as (linear) search on all values in collection.
     * @param slot slot to compare against
     * @return -1 if none is found, index otherwise
     */
    ssize_t find_existing_soundslot_(const soundslot& slot);

    /**
     * Register a sound in the game (map sounds variant).
     * @param name path to the sound file (relative to data directory)
     * @param volume volume sound is played with (see soundslot)
     * @param max_radius maximum radius sound is played in (see soundslot)
     * @param min_radius minimum radius sound is played in (see soundslot)
     * @param num_slots amount of slots to load for sound "name"
     * @param sound_index index of sound in the sounds enum (must be -1 for map sounds, and >= 0 for game sounds)
     * @return index of registered sound in internal storage (also if the sound was not added because it existed already)
     */
    size_t insert_or_append_(const std::string& name, int volume, int max_radius, int min_radius, int num_slots, ssize_t index);

    /**
     * Type-specific insertion method.
     * @param index -1 for vector types, actual index for map types
     * @param slot soundslot to insert/append
     */
    size_t insert_into_soundslots_(ssize_t index, const soundslot& slot);

public:
    /**
     * Default constructor.
     */
    soundslot_collection();

    /**
     * Register a sound in the game (mapsounds/vector variant).
     * @param name path to the sound file (relative to data directory)
     * @param volume volume sound is played with (see soundslot)
     * @param max_radius maximum radius sound is played in (see soundslot)
     * @param min_radius minimum radius sound is played in (see soundslot)
     * @param num_slots amount of slots to load for sound "name"
     * @param sound_index index of sound in the sounds enum (must be -1 for map sounds, and >= 0 for game sounds)
     * @return index of registered sound in internal storage (also if the sound was not added because it existed already)
     */
    size_t append(const std::string& name, int volume, int max_radius, int min_radius, int num_slots);

    /**
     * Register a sound in the game (gamesounds/ variant).
     * @param name path to the sound file (relative to data directory)
     * @param volume volume sound is played with (see soundslot)
     * @param max_radius maximum radius sound is played in (see soundslot)
     * @param min_radius minimum radius sound is played in (see soundslot)
     * @param num_slots amount of slots to load for sound "name"
     * @param sound_index index of sound in the sounds enum (must be -1 for map sounds, and >= 0 for game sounds)
     * @return index of registered sound in internal storage (also if the sound was not added because it existed already)
     */
    size_t insert(size_t index, const std::string& name, int volume, int max_radius, int min_radius, int num_slots);

    /**
     * Clear internal collection.
     */
    void clear();

    /**
     * Query entry for index i.
     * @param i index
     * @return soundslot entry
     */
    const soundslot& operator[](size_t i) const;

    /**
     * Query entry for index i.
     * @param i index
     * @return soundslot entry
     */
    soundslot& operator[](size_t i);

    /**
     * Checks whether soundslot with index i is contained in internal collection.
     * @param i index
     * @return whether i is contained in internal collection
     */
    bool contains(size_t i) const;

    /**
     * Get size of internal collection. Available only for vector implementation.
     * @return size of internal collection
     */
    size_t size() const;
};

template<typename T>
bool soundslot_collection<T>::check_soundslots_equal_(const soundslot& first, const soundslot& second) {
    return first.vol == second.vol &&
           first.maxrad == second.maxrad &&
           first.minrad == second.minrad &&
           strcmp(first.name, second.name) == 0;
}

template<typename T>
soundslot_collection<T>::soundslot_collection() : soundslots_() {
    static_assert(
        std::is_same<T, soundslot_collection_map_t>::value || std::is_same<T, soundslot_collection_vector_t>::value,
        "unsupported type for soundslot collection"
    );
}

template<typename T>
size_t soundslot_collection<T>::insert_or_append_(const std::string& name, int volume, int max_radius, int min_radius, int num_slots, ssize_t index) {
    // sanitization: bound value and use max when a negative volume is passed
    if (volume <= 0 || volume >= 255) {
        volume = 255;
    }

    // more sanitization: bound values to -1 on the negative side
    min_radius = std::max(-1, min_radius);
    max_radius = std::max(-1, max_radius);

    // there should be at least one slot for every sound
    num_slots = std::max(1, num_slots);

    // prepare new soundslot to be added
    // also used to compare against existing ones (see below)
    soundslot new_slot{};
    new_slot.name = newstring(name.c_str());
    new_slot.vol = volume;
    new_slot.maxrad = max_radius;
    new_slot.minrad = min_radius;

    // TODO: validate whether this check is needed at all; all slots should be filled once the sound has been registered
    // check if soundslot has been loaded already to avoid loading samples more than once
    if (num_slots == 1) {
        const auto existing_slot_index = find_existing_soundslot_(new_slot);

        // if an existing soundslot is found, it is copied into the new location
        if (existing_slot_index >= 0) {
            return insert_into_soundslots_(index, (*this)[existing_slot_index]);
        }
    }

    // just used for weapons that don't have certain sound effects, e.g. some weapons don't have a zoom sound
    // so we don't create an offset inside the vector, we have to fill those places with some
    // placeholders
    if (name == "<none>") {
        throw std::invalid_argument{"cubescript relict; should not be used any more"};
    }

    // load the actual samples here
    int loaded_samples = 0;

    // naming scheme is: shell.ogg, shell2.ogg, shell3.ogg, shell4.ogg
    // the values would be name: "shell" and value: 4
    // so the first element has to always be loaded, that's why do...while
    // after that just go on counting up 'til we reach the value and load all sounds we find on our way
    for (ssize_t i = 1; i <= num_slots; ++i) {
        std::string sample_name = name;

        // if the value is 1 (the first entry) just use the normal name since there is no such thing as shell1.ogg,
        // but rather shell.ogg and then it continues with shell2.ogg (that's why we start counting from 1)
        if (i > 1) {
            sample_name += std::to_string(i);
        }

        soundsample* sample = nullptr;

        if ((sample = soundsamples.access(sample_name.c_str())) == nullptr) {
            char* s_name = newstring(sample_name.c_str());

            sample = &soundsamples[s_name];
            sample->name = s_name;
            sample->sound = nullptr;
        }

        // if the sample isn't already loaded, load it
        if (!sample->sound) {
            for (const auto& dir : {"", "sounds/"}) {
                for (const auto& ext : {"", ".wav", ".ogg"}) {
                    std::string relative_path = dir + sample_name + ext;
                    sample->sound = loadwav(relative_path.c_str());

                    // if we can load the sample, set found to true and the exit the loop
                    if (sample->sound != nullptr) {
                        break;
                    }
                }

                // if we found the entry previously, quit the loop
                if (sample->sound != nullptr) {
                    break;
                }
            }
        }

        // check if the sample is set now
        if (sample->sound != nullptr) {
            new_slot.samples.emplace_back(sample);
            loaded_samples++;
        }
    }

    if (loaded_samples < num_slots) {
        std::string message = "failed to load " + std::to_string(num_slots - loaded_samples) + " samples for sound " +  name;
        throw std::invalid_argument(message.c_str());
    }

    printf("loaded %d samples for sound %d: %s\n", num_slots, index, new_slot.name);
    return insert_into_soundslots_(index, new_slot);
}

template<typename T>
void soundslot_collection<T>::clear() {
    soundslots_.clear();
}

template<typename T>
const soundslot& soundslot_collection<T>::operator[](size_t i) const {
    return this->soundslots_[i];
}

template<typename T>
soundslot& soundslot_collection<T>::operator[](size_t i) {
    return this->soundslots_[i];
}
