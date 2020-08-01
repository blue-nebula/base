#include "soundsample.h"

soundsample::soundsample() : name(nullptr) {}

soundsample::~soundsample() {
    free(name);
}

void soundsample::cleanup()
{
    Mix_FreeChunk(sound);
    sound = nullptr;
}
