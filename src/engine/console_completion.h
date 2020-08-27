#include "console.h"
#include "engine.h"

//////////////////////////
/// COMMAND COMPLETION ///
//////////////////////////
class CommandCompletionEntry : public CompletionEntryBase
{
public:
    ident id;
    virtual std::string get_title();
    virtual std::string get_description();
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
