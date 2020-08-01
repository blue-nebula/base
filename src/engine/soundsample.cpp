// system headers
#include <cstdlib>

// local headers
#include "soundsample.h"

soundsample::soundsample() : sound(nullptr), name(nullptr) {}

soundsample::~soundsample() {
    free(name);
}

void soundsample::cleanup()
{
    Mix_FreeChunk(sound);
    sound = nullptr;
}
