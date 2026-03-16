#pragma once

#include "SDL_mixer.h"

struct soundsample
{
    Mix_Chunk *sound;
    char *name;

    soundsample();
    ~soundsample();

    void cleanup();
};
