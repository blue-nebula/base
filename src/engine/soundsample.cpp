// system headers
#include <cstdlib>

// local headers
#include "soundsample.h"
#include "cube.h"

soundsample::soundsample() : sound(nullptr), name(nullptr) {}

soundsample::~soundsample() {
    DELETEA(name);
}

void soundsample::cleanup()
{
    Mix_FreeChunk(sound);
    sound = nullptr;
}
