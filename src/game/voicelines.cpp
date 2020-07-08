#include <algorithm>
using std::swap;
#include "game.h"
#include "voicelines.h"

namespace voicelines
{
    std::map<const int, voiceline> voiceline_list {
        // please add one space in front and after the actual trigger word itself so the detection is not triggered by accident e.g.
        // "no" being triggered by "knowledge"
        {S_VL_NO,        voiceline("voice/no"       , 2, std::vector<std::string>{" no", " gno "})                               },
        {S_VL_LUCKYSHOT, voiceline("voice/luckyshot", 2, std::vector<std::string>{" lucky shot ", " ls "})                        },
        {S_VL_NICESHOT,  voiceline("voice/niceshot" , 2, std::vector<std::string>{" nice shot", " ns "})                         },
        {S_VL_NOPROBLEM, voiceline("voice/noproblem", 2, std::vector<std::string>{" no problem ", " np ", " no prob ", " gno prob "}) },
        {S_VL_YES,       voiceline("voice/yes"      , 2, std::vector<std::string>{" yes "})                                     },
        {S_VL_SORRY,     voiceline("voice/sorry"    , 2, std::vector<std::string>{" sorry " , " sry "})                            },
        {S_VL_ARGH,      voiceline("voice/argh"     , 2, std::vector<std::string>{" argh "})                                     },
        {S_VL_BOOM,      voiceline("voice/boom"     , 2, std::vector<std::string>{" boom "})                                     },
        {S_VL_DAMNIT,    voiceline("voice/damnit"   , 2, std::vector<std::string>{" damnit "})                                   },
        {S_VL_HAHA,      voiceline("voice/haha"     , 2, std::vector<std::string>{" haha "})                                     },
        {S_VL_SUCKIT,    voiceline("voice/suckit"   , 2, std::vector<std::string>{" suck "})                                     },
        {S_VL_PZAP,      voiceline("voice/pzap"     , 2, std::vector<std::string>{" pzap "})                                     },
        {S_VL_GOGOGO,    voiceline("voice/gogogo"   , 2, std::vector<std::string>{" go go go ", " gogogo "})                       },
        {S_VL_HANGON,    voiceline("voice/hangon"   , 2, std::vector<std::string>{" hangon ", " hang on "})                                   },
        {S_VL_THANKS,    voiceline("voice/thanks"   , 2, std::vector<std::string>{" thanks ", " ty ", " thx ", " thank you "})                    }
    };

    int try_get_voiceline_sound(std::string text)
    {
        int sound = -1;
        // modify text so trigger words like " gno " can still work even if the text is equal to a single world like e.g. "gno"
        text = " " + text + " ";

        for (auto voiceline : voiceline_list)
        {
            for (std::string trigger_word : voiceline.second.trigger_words)
            {
                if (text.find(trigger_word) != std::string::npos)
                {
                    sound = voiceline.first;
                }
            }
        }

        return sound;
    }
};
