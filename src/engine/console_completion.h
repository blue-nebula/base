#include "console.h"
#include "engine.h"
#include <array>

//////////////////////////
/// COMMAND COMPLETION ///
//////////////////////////
class CommandCompletionEntry : public CompletionEntryBase
{
private:
    std::string arguments_string;
    std::string title_string;
    std::string description_string;
public:
    ident id;
    CommandCompletionEntry(ident id, int completion_length);

    virtual std::string get_icon();
    virtual int get_icon_color();
    virtual std::string get_title();
    virtual std::string get_description();

    std::array<std::string, 4> get_ident_values();
    std::string get_ident_args_text();
    std::string get_ident_flags_text();
};

class CommandCompletion : public CompletionBase
{
private:
    virtual int stick_to_buffer_idx();
    virtual bool can_complete(Console& console);
    virtual std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    virtual void select_entry(CompletionEntryBase* entry, Console& console);
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

    virtual std::string get_icon();
    virtual int get_icon_color();
    virtual std::string get_title();
    virtual std::string get_description();
};

class PlayerNameCompletion : public CompletionBase
{
private:
    int stick_to_buffer_pos = 0;
    virtual int stick_to_buffer_idx();
    virtual bool can_complete(Console& console);
    virtual std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    virtual void select_entry(CompletionEntryBase* entry, Console& console);
};

class MapNameCompletionEntry : public CompletionEntryBase
{
public:
    std::string name;
    std::string icon;
    std::string title;

    MapNameCompletionEntry(std::string name, std::string icon, int completion_length);

    virtual int get_icon_color();
    virtual std::string get_icon();
    virtual std::string get_title();
    virtual std::string get_description();
};

class MapNameCompletion : public CompletionBase
{
private:
    const std::string prefix = ":m";

    virtual int stick_to_buffer_idx();
    virtual bool can_complete(Console& console);
    virtual std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    virtual void select_entry(CompletionEntryBase* entry, Console& console);
};

