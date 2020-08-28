#include "console_completion.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

void remove_trailing_zeros(std::string& s)
{
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);

    // if the input was smth like "1.00000" it'll end up being
    // "1.", thus check if last character is a '.' and add a '0' to get "1.0"
    if (s.back() == '.')
    {
        s += '0';
    }
}


//////////////////////////
/// COMMAND COMPLETION ///
//////////////////////////

struct SortCommandCompletionEntry
{
    inline bool operator() (CompletionEntryBase* e1, CompletionEntryBase* e2)
    {
        CommandCompletionEntry* c1 = (CommandCompletionEntry*)e1;
        CommandCompletionEntry* c2 = (CommandCompletionEntry*)e2;
        return strlen(c1->id.name) < strlen(c2->id.name);
    }
};

//TODO: find better way to convert from decimal to hex
template<typename T>
std::string dec_to_hex(T i, bool include_alpha = false)
{
    std::stringstream stream;
    stream << "0x"
        << std::setfill('0') << std::setw(sizeof(T) * 2)
        << std::hex << i;
    std::string hex_string = stream.str();
    // make entire string uppercase
    for (auto& c : hex_string)
    {
        c = toupper(c);
    }

    if (!include_alpha && hex_string.length() > 8)
    {
        hex_string = "0x" + hex_string.substr(4, 10);
    }
    return hex_string;
}

CommandCompletionEntry::CommandCompletionEntry(ident id, int completion_length)
{
    this->id = id;
    this->completion_length = completion_length;
  
    static const std::string COLOR_A = "\fa";
    static const std::string COLOR_G = "\fg";
    static const std::string COLOR_W = "\fw";
    static const char LINE_BREAK = '\n';

    /// TITLE ///
    /////////////
    title_string = COLOR_G + id.name;
    // + 2 because we have to add 3 due to adding \fg before, and we have to substract 1
    title_string.insert(completion_length + 2, COLOR_W);
  
    // arguments
    std::string args_text = get_ident_args_text();
    // check if args_text contains any non-space characters, as it's " " by default
    // so args_text.empty() wouldn't do anything
    if (args_text.find_first_not_of(' ') != std::string::npos)
    {
        title_string += COLOR_A + args_text;
    }

    /// DESCRIPTION ///
    ///////////////////
    description_string = "";
  
    std::array<std::string, 4> values = get_ident_values();

    static const std::string TEXT_CURRENT = "\fwCurrent: \fy";
    static const std::string TEXT_MIN = " \fwMin: \fb";
    static const std::string TEXT_MAX = " \fwMax: \fr";
    static const std::string TEXT_RANGE_START = " \fwRange: \fb";
    static const std::string TEXT_RANGE_CENTER = "\fw -> \fr";
    static const std::string TEXT_RANGE_END = "\fw";
    static const std::string TEXT_DEFAULT = " \fwDefault: \fg";
    static const std::string TEXT_NO_DESC = "No Description Available.";

    switch (id.type)
    {
        case ID_VAR:
        case ID_FVAR:
            description_string = TEXT_CURRENT + values[3]
                                 + TEXT_DEFAULT + values[2]
                                 + TEXT_RANGE_START + values[0] 
                                 + TEXT_RANGE_CENTER + values[1]
                                 + TEXT_RANGE_END + LINE_BREAK;
            break;
        case ID_SVAR:
            description_string = TEXT_CURRENT \
                                 + values[2] + TEXT_DEFAULT \
                                 + values[3] + LINE_BREAK;
            break;
    }

    static const std::string TEXT_FLAGS = "\fwFlags: \fa";
    std::string flags = get_ident_flags_text();
    if (!flags.empty())
    {
        description_string += TEXT_FLAGS + flags + LINE_BREAK;
    }

    if (id.desc != nullptr && *id.desc != 0)
    {
        description_string += id.desc;
    }
    else
    {
        description_string += TEXT_NO_DESC;
    } 
}

std::array<std::string, 4> CommandCompletionEntry::get_ident_values()
{
    std::array<std::string, 4> values;

    switch (id.type)
    {
        case ID_VAR:
            if (id.flags & IDF_HEX)
            {
                // convert int to hex
                values[0] = dec_to_hex(id.minval);
                values[1] = dec_to_hex(id.maxval);
                values[2] = dec_to_hex(id.def.i);
                values[3] = dec_to_hex(*id.storage.i);
            }
            else
            {
                values[0] = id.minval == VAR_MIN ? "INT MIN" : std::to_string(id.minval);
                values[1] = id.maxval == VAR_MAX ? "INT MAX" : std::to_string(id.maxval);
                values[2] = std::to_string(id.def.i);
                values[3] = std::to_string(*id.storage.i);
            }
            break;
        case ID_FVAR:
            values[0] = id.minvalf == FVAR_MIN ? "FLOAT MIN" : std::to_string(id.minvalf);
            values[1] = id.maxvalf == FVAR_MAX ? "FLOAT MAX" : std::to_string(id.maxvalf);
            values[2] = std::to_string(id.def.f);
            values[3] = std::to_string(*id.storage.f);
            // remove trailing 0s
            remove_trailing_zeros(values[0]);
            remove_trailing_zeros(values[1]);
            remove_trailing_zeros(values[2]);
            remove_trailing_zeros(values[3]);
            break;
        case ID_SVAR:
            values[2] = id.def.s;
            values[3] = *id.storage.s;
            break;
    }

    return values;
}

std::string CommandCompletionEntry::get_ident_args_text()
{
    std::string args = "";
        
    switch (id.type)
    {
        case ID_ALIAS:
            args = " <arguments>";
            break;
        case ID_VAR:
            args = " (integer)";
            break;
        case ID_FVAR:
            args = " (float)";
            break;
        case ID_SVAR:
            args = " (string)";
            break;
        case ID_COMMAND:
            if (id.args == nullptr)
            {
                break;
            }

            for (int i = 0; i < int(strlen(id.args)); ++i)
            {
                args += " ";
                switch(id.args[i])
                {
                    case 's':
                        args += "<string>";
                        break;
                    case 'i':
                    case 'b':
                    case 'N':
                        args += std::string("<")
                                + (id.flags & IDF_HEX ? "hex" : "int")
                                + std::string(">");
                        break;
                    case 'f':
                    case 'g':
                        args += "<float>";
                        break;
                    case 't':
                        args += "<null>";
                        break;
                    case 'e':
                        args += "<commands>";
                        break;
                    case 'r':
                    case '$':
                        args += "<ident>";
                        break;
                    default:
                        args += "<?>";
                        break;
                }
            }
    }

    return args;
}

std::string CommandCompletionEntry::get_ident_flags_text()
{
    std::string flags = "";
    
    if (id.flags & IDF_PERSIST)   { flags += "persistent, ";     }
    if (id.flags & IDF_READONLY)  { flags += "read-only, ";      }
    if (id.flags & IDF_CLIENT)    { flags += "client, ";         }
    if (id.flags & IDF_SERVER)    { flags += "server, ";         }
    if (id.flags & IDF_WORLD)     { flags += "world, ";          }
    if (id.flags & IDF_ADMIN)     { flags += "admin-only, ";     }
    if (id.flags & IDF_MODERATOR) { flags += "moderator-only, "; }
    if (flags != "")
    {
        // cut away the ", " at the end
        flags = flags.substr(0, flags.length() - 2);
    }
    return flags;
}


std::string CommandCompletionEntry::get_title()
{
    return title_string;
}

std::string CommandCompletionEntry::get_description()
{  
    return description_string;
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
                results.push_back(new CommandCompletionEntry(id, search_term.length()));
            }
        }
    }
    // sort by length of the name, shortest to longest
    std::sort(results.begin(), results.end(), SortCommandCompletionEntry());

    return results;
}

void CommandCompletion::select_entry(CompletionEntryBase* entry, Console& console)
{
    printf("Selected an entry, lmao\n");
}

//////////////////////////////
/// PLAYER NAME COMPLETION ///
//////////////////////////////

PlayerNameCompletionEntry::PlayerNameCompletionEntry(std::string name, int completion_length)
{
    this->name = name;
    this->completion_length = completion_length;
}

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
            results.push_back(new PlayerNameCompletionEntry(game::players[i]->name, entered_name.length()));
        }
    }

    return results;
}

void PlayerNameCompletion::select_entry(CompletionEntryBase* entry, Console& console)
{
    // use str.find_last_of
    
}
