#include "console_completion.h"

//////////////////////////
/// COMMAND COMPLETION ///
//////////////////////////
std::string CommandCompletionEntry::get_title()
{
    return id.name;
}

std::string CommandCompletionEntry::get_description()
{
    return "Test Description";
}

bool CommandCompletion::can_complete(Console& console)
{
    const std::string buffer = console.get_buffer();
    if (!buffer.empty())
    {
        if (buffer[0] == console.command_prefix)
        {
            return true;
        }
    }
    return false;
}

std::vector<CompletionEntryBase*> CommandCompletion::get_completions(const std::string buffer)
{
    std::vector<CompletionEntryBase*> results;

    // only the first word matters
    size_t cut_off_pos = buffer.find(' ');
    if (cut_off_pos == std::string::npos)
    {
        cut_off_pos = -1;
    }
    else
    {
        cut_off_pos--;
    }

    std::string search_term = buffer.substr(1, cut_off_pos); 

    if (search_term.length() == 0)
    {
        return results;
    }

    // this is just what enumerate usually does
    // I am not sure what it does exactly, but it works :/
    for (int i = 0; i < idents.size; ++i)
    {
        for (void* ec = idents.chains[i]; ec;)
        {
            const ident& id = idents.enumdata(ec);
            ec = idents.enumnext(ec);

            std::string id_name = id.name;
            if (   (id.type == ID_ALIAS && !(id.flags & IDF_COMPLETE) 
                || id.type == IDF_TEXTURE)
                || id_name.length() < search_term.length())
            {
                continue;
            }

            // only iterate through server vars if player1 has required privs
            if (id_name.find(search_term) == 0)
            {
                CommandCompletionEntry* entry = new CommandCompletionEntry();
                entry->id = id;
                results.push_back(entry);
            }
        }
    }

    return results;
}

void CommandCompletion::select_entry(CompletionEntryBase* entry, Console& console)
{
    printf("Selected an entry, lmao\n");
}

//////////////////////////////
/// PLAYER NAME COMPLETION ///
//////////////////////////////

std::string PlayerNameCompletionEntry::get_title()
{
    return name;
}

std::string PlayerNameCompletionEntry::get_description()
{
    return "";
}


#include "game.h"
bool PlayerNameCompletion::can_complete(Console& console)
{
    const std::string buffer = console.get_buffer();
    return buffer.find_last_of(console.playername_prefix) != std::string::npos;
}

std::vector<CompletionEntryBase*> PlayerNameCompletion::get_completions(const std::string buffer)
{
    std::vector<CompletionEntryBase*> results;

    size_t prefix_pos = buffer.find_last_of('@');
    // this shouldn't be necessary, but for now I'll leave it in
    if (prefix_pos == std::string::npos)
    {
        return results;
    }

    //TODO: instead of using -1, use current cursor_pos, that way completion can be
    //based on cursor position instead of only being possible on the end of the input
    std::string entered_name = buffer.substr(prefix_pos + 1, -1);
    printf("Entered name: %s\n", entered_name.c_str());

    for (int i = 0; i < game::players.length(); i++)
    {
        // we want to exclude AI from the completion
        // and there is no need to check if it fits if the name of the player
        // is smaller than the entered text
        if (   game::players[i] == nullptr 
            //|| game::players[i]->actortype == ENT_AI
            || strlen(game::players[i]->name) < entered_name.length())
        {
            continue;
        }
        // check if the playername fits the input
        bool fits = std::string(game::players[i]->name).find(entered_name) == 0;
        if (fits)
        {
            PlayerNameCompletionEntry* entry = new PlayerNameCompletionEntry();
            entry->name = game::players[i]->name;
            results.push_back(entry);
        }
    }

    return results;
}

void PlayerNameCompletion::select_entry(CompletionEntryBase* entry, Console& console)
{
    // use str.find_last_of
    
}
