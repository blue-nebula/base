#pragma once

// system headers
#include <vector>

// local headers
#include "soundsample.h"

struct soundslot
{
    std::vector<soundsample*> samples;
    int vol, maxrad, minrad;
    char *name;

    soundslot();
    ~soundslot();
};
