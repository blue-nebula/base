#include "console.h"
#include "engine.h"
#include <array>

//////////////////////////
/// COMMAND COMPLETION ///
//////////////////////////
class CommandCompletionEntry : public CompletionEntryBase
{
public:
    std::string arguments_string;
    std::string title_string;
    std::string description_string;
    
    ident id;
    CommandCompletionEntry(ident id, int completion_length);

    std::string get_icon();
    int get_icon_color();
    std::string get_title();
    std::string get_description();

    std::array<std::string, 4> get_ident_values();
    std::string get_ident_args_text();
    std::string get_ident_flags_text();
};

class CommandCompletion : public CompletionEngineBase
{
public:
    int stick_to_buffer_idx();
    bool can_complete(Console& console);
    std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    void select_entry(CompletionEntryBase* entry, Console& console);
};


//////////////////////////////
/// PLAYER NAME COMPLETION ///
//////////////////////////////
class PlayerNameCompletionEntry : public CompletionEntryBase
{
public:
    std::string name;
    std::string icon;
    int icon_color;

    PlayerNameCompletionEntry(std::string name, std::string icon, int icon_color, int completion_length);

    std::string get_icon();
    int get_icon_color();
    std::string get_title();
    std::string get_description();
};

class PlayerNameCompletion : public CompletionEngineBase
{
private:
    static const char prefix = '@';
    int stick_to_buffer_pos = 0;

public:
    int stick_to_buffer_idx();
    bool can_complete(Console& console);
    std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    void select_entry(CompletionEntryBase* entry, Console& console);
};

class MapNameCompletionEntry : public CompletionEntryBase
{
public:
    std::string name;
    std::string icon;
    std::string title;

    MapNameCompletionEntry(std::string name, std::string icon, int completion_length);

    int get_icon_color();
    std::string get_icon();
    std::string get_title();
    std::string get_description();

};

class MapNameCompletion : public CompletionEngineBase
{
private:
    const std::string prefix = ":m";
public:
    int stick_to_buffer_idx();
    bool can_complete(Console& console);
    std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    void select_entry(CompletionEntryBase* entry, Console& console);
};

