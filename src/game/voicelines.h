#ifndef VOICELINES_H
#define VOICELINES_H

#include <map>
#include <vector>
#include <string>

namespace voicelines
{

    class voiceline
    {
    public:
        voiceline(std::string path, int sound_variants, std::vector<std::string> trigger_words) :
        path(path), sound_variants(sound_variants), trigger_words(trigger_words) {}

        std::string path;
        int sound_variants;
        std::vector<std::string> trigger_words;
    };

    extern std::map<const int, voiceline> voiceline_list;
    extern int try_get_voiceline_sound(std::string text);
};

#endif
