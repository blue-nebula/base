#pragma once

// system headers
#include <string>
#include <vector>

// local headers
#include "soundsample.h"

struct soundslot
{
    std::vector<soundsample*> samples;
    int vol, maxrad, minrad;
    char *name;

    soundslot();
    soundslot(const soundslot& other);
    soundslot& operator=(const soundslot& other);
    ~soundslot();
};
