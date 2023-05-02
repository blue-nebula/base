// local headers
#include "soundslot.h"
#include "cube.h"

soundslot::soundslot() : samples(), vol(255), maxrad(-1), minrad(-1), name(nullptr) {}

soundslot::~soundslot() {
    DELETEA(name);
    name = nullptr;
}

soundslot::soundslot(const soundslot& other) {
    samples = other.samples;
    vol = other.vol;
    maxrad = other.maxrad;
    minrad = other.minrad;
    name = newstring(other.name);
}

soundslot& soundslot::operator=(const soundslot& other) {
    if (this != &other) {
        samples = other.samples;
        vol = other.vol;
        maxrad = other.maxrad;
        minrad = other.minrad;
        name = newstring(other.name);
    }

    return *this;
}
