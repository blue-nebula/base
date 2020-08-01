#include "soundslot.h"

soundslot::soundslot() : samples(), vol(255), maxrad(-1), minrad(-1), name(nullptr) {}

soundslot::~soundslot() {
    free(name);
    name = nullptr;
}
