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

    virtual std::string get_title();
    virtual std::string get_description();

    std::array<std::string, 4> get_ident_values();
    std::string get_ident_args_text();
};

class CommandCompletion : public CompletionBase
{
private:
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
    
    PlayerNameCompletionEntry(std::string name, int completion_length);

    virtual std::string get_title();
    virtual std::string get_description();
};

class PlayerNameCompletion : public CompletionBase
{
private:
    virtual bool can_complete(Console& console);
    virtual std::vector<CompletionEntryBase*> get_completions(const std::string buffer);
    virtual void select_entry(CompletionEntryBase* entry, Console& console);
};
