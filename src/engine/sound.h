#pragma once

#include <string>
#include <map>
#include <vector>

// forward declarations
struct physent;

enum
{
    S_GUIPRESS = 0, S_GUIBACK, S_GUIACT,
    S_GAMESPECIFIC
};

enum
{
    SND_NONE    = 0,
    SND_NOATTEN = 1<<0, // disable attenuation
    SND_NODELAY = 1<<1, // disable delay
    SND_NOCULL  = 1<<2, // disable culling
    SND_NOPAN   = 1<<3, // disable panning (distance only attenuation)
    SND_NODIST  = 1<<4, // disable distance (panning only)
    SND_NOQUIET = 1<<5, // disable water effects (panning only)
    SND_CLAMPED = 1<<6, // makes volume the minimum volume to clamp to
    SND_LOOP    = 1<<7, // loops when it reaches the end
    SND_BUFFER  = 1<<8, // channel becomes/adds to a buffer for sounds
    SND_MAP     = 1<<9,
    SND_IMPORT  = SND_NODELAY|SND_NOCULL|SND_NOQUIET,
    SND_FORCED  = SND_IMPORT|SND_NOATTEN|SND_NODIST,
    SND_DIRECT  = SND_IMPORT|SND_CLAMPED,
    SND_MASKF   = SND_LOOP|SND_MAP,
    SND_LAST    = 7
};

#ifndef STANDALONE
#include "geom.h"
#include "soundslot.h"
#include "soundsample.h"
#include "soundslot_collection.h"

#define SOUNDMINDIST        16.0f
#define SOUNDMAXDIST        10000.f

struct sound
{
    soundslot *slot;
    vec pos, oldpos;
    physent *owner;
    int vol, curvol, curpan;
    int flags, maxrad, minrad, material;
    int millis, ends, slotnum, chan, *hook;
    vector<int> buffer;

    sound();
    ~sound();

    void reset();
    bool playing();
    bool valid() { return chan >= 0 && playing(); }
};

extern bool nosound;
extern int mastervol, soundvol, musicvol;

/*
 * For sounds, it makes sense to use a map type to manage the association between the int key (taken from enums) and a
 * soundslot value.
 * The only problem is that an unordered_map cannot simply be used, as ints are an unhashable type. Sure, this could
 * be worked around on, but for now there is no need to optimize to get this sub-1% performance bonus.
 * It is planned to put a proper interface in front of the soundslot collections that allows for accessing them easily
 * and safely. That will also allow using better (read: more performant) data structures (e.g., an std::vector combined
 * with support for the null object pattern, or an actual std::unordered_map with a custom hash function).
 */
extern soundslot_collection<soundslot_collection_map_t> gamesounds;

/*
 * Map sounds are added every time a map is loaded.
 */
extern soundslot_collection<soundslot_collection_vector_t> mapsounds;

extern vector<sound> sounds;

#define issound(c) (sounds.inrange(c) && sounds[c].valid())

extern void initsound();
extern void stopsound();
extern bool playmusic(const char *name, const char *cmd = NULL);
extern bool playingmusic(bool check = true);
extern void smartmusic(bool cond, bool init = false);
extern void musicdone(bool docmd);
extern void updatesounds();
extern size_t add_game_sound(size_t sound_index, const std::string& name, int volume = 0, int max_radius = 0, int min_radius = 0, int num_slots = 1);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, const vec &pos, physent *d = NULL, int flags = 0, int vol = -1, int maxrad = -1, int minrad = -1, int *hook = NULL, int ends = 0, int *oldhook = NULL);
extern void removetrackedsounds(physent *d);
extern void initmumble();
extern void closemumble();
extern void updatemumble();
#endif
