#include <algorithm>
using std::swap;

#include "completions.h"
#include "game.h"

namespace completion
{
    std::string completed_word = " ";
    std::vector<Completion> available_completions {};
    int completion_pos = 0;
    int selected_completion = 0;


    template<typename T>
    std::string int_to_hex(T i, bool include_alpha = false)
    {
        std::stringstream stream;
        stream << "0x"
                << std::setfill ('0') << std::setw(sizeof(T)*2)
                << std::hex << i;
        std::string hex_string = stream.str();
        // make entire string uppercase
        for (auto& c : hex_string) {
            c = toupper(c);
        }

        if (!include_alpha && hex_string.length() > 8) {
            hex_string = "0x" + hex_string.substr(4, 10);
        }

        return hex_string;
    }



    /**
     * @brief Check if there are completions available
     * 
     * @return bool does available_completions have more than 0 elements
     */
    bool completions_available() {
        return available_completions.size() > 0;
    }



    /**
     * @brief Increase selected_completion by 1 if possible
     * 
     * sets selected_completion to max(selected_completion - 1, 0)
     * if there are any completions_available()
     * 
     * @return was selected_completion altered inside this function?
     */
    bool move_completion_select_up() 
    {
        // we don't want to block the normal history scrolling, so you can only go up in the completions if
        // you've moved down before
        if (completions_available())
        {
            int old_pos = selected_completion;
            selected_completion = std::max(selected_completion - 1, 0);
            
            return old_pos != selected_completion;
        }
        return false;
    }



    /**
     * @brief Decrease selected_completion by 1 if possible
     * 
     * Sets selected_completion to min(selected_completion + 1, available_completions.size() - 1)
     * if there are any completions_available()
     * 
     * @return was selected_completion altered inside this function?
     */
    bool move_completion_select_down()
    {
        if (completions_available())
        {
            int old_pos = selected_completion;
            selected_completion = std::min(selected_completion + 1, int(available_completions.size()) - 1);
            
            return old_pos != selected_completion;
        }
        return false;
    }



    /**
     * @brief Check if selected_completion is inside the bounds of available_completions
     * 
     * @return is selected_completion smaller than available_completions.size() and bigger than -1
     */
    bool selected_completion_is_available()
    {
        return (selected_completion < int(available_completions.size()) && selected_completion > -1);
    }



    /**
     * @brief Checks if text qualifies as a command
     * 
     * text is a command if it's bigger than 0 and starts with '/'
     * 
     * @param text text to be checked
     * @return does text qualify as a command?
     */
    bool is_command(const std::string& text)
    {
        if (text.length() > 0)
        {
            if (text[0] == '/') {
                return true;
            }
        }
        
        return false;
    }



    /**
     * @brief Check if text could contain a completion
     * 
     * @param text text to be checked
     * @return is text a command or does it contain '@'?
     */
    bool could_contain_completion(const std::string& text)
    {
        return (is_command(text) || get_name_pos(text) != std::string::npos);
    }



    /**
     * @brief Get the start of a name completion indicator symbol ('@') inside of text
     * 
     * Does a reverse search for '@' inside text
     * 
     * @param text text to be checked
     * @return result of reverse search for '@' inside text
     */
    size_t get_name_pos(const std::string& text)
    {
        // we are only interested in the last occurrence of @, that's why we use
        // rfind instead of find
        return text.rfind('@');
    }



    /**
     * @brief Fills auto_completions vector with completions that were found for text
     * 
     * Fills auto_completions with either identifier completions or name completions.
     */
    void get_completions(const std::string& text)
    {
        if (text.length() == 0) {
            selected_completion = 0;
            available_completions.clear();
        }

        if (!could_contain_completion(text)) {
            return;
        }

        size_t name_start_pos = get_name_pos(text);
        
        if (name_start_pos != std::string::npos) {
            // -- complete names

            // reset available_completions
            selected_completion = 0;
            available_completions.clear();

            // we are not interested in the @ itself
            name_start_pos++;

            std::string name = text.substr(name_start_pos, text.length() - 1);

            if (completed_word == name) {
                return;
            }

            // do not check if name.length() > 0 so if you don't type any name, you just get an overview of all names

            for (int i = 0; i < game::players.length(); i++)
            {
                if (game::players[i] == nullptr) {
                    continue;
                }
                // create an std::string version of the player name so
                // we can use find
                std::string player_name = game::players[i]->name;

                // if the entered name is longer than the playername, don't even bother with it
                if (name.length() > player_name.length()) {
                    continue;
                }

                // exclude bots
                if (game::players[i]->actortype == ENT_AI) {
                    //continue;
                }

                if (player_name.find(name) == 0) {
                    available_completions.push_back(Completion(player_name));

                    if (int(available_completions.size()) == max_completions) {
                        break;
                    }
                }
            }
            std::reverse(std::begin(available_completions), std::end(available_completions));

            completed_word = name;
            completion_pos = text.length() - 1;

            return;
        }

        // -- complete identifiers

        // it's just the first word that matters
        size_t first_space_pos = text.find(' ');
        if (int(first_space_pos) != -1) {
            first_space_pos--;
        }
        // start from 1 instead of 0 to cut away the /
        std::string word = text.substr(1, first_space_pos);

        // prevent checking the same word multiple times
        if (word != completed_word && word.length() != 0)
        {
            selected_completion = 0;
            available_completions.clear();
            bool done = false;

            // this is just what enumerate usually does
            // I am not sure what it exactly means, but I know it does what I need
            for (int i = 0; i < idents.size; ++i)
            {
                for (void* ec = idents.chains[i]; ec;)
                {
                    const ident& id = idents.enumdata(ec);
                    ec = idents.enumnext(ec);

                    // put id.name into std::string so we can utilize the std::string::find function
                    std::string id_name = id.name;
                    if ((id.type == ID_ALIAS && !(id.flags & IDF_COMPLETE)) || id.type == IDF_TEXTURE) {
                        continue;
                    }

                    // only enumerate through server vars if player1 has required privs
                    if (id_name.find(word) == 0) 
                    {
                        available_completions.push_back(Completion(id));

                        if (int(available_completions.size()) == max_completions) {
                            done = true;
                            break;
                        }
                    }
                }

                if (done) {
                    break;
                }
            }
            std::reverse(std::begin(available_completions), std::end(available_completions));
        }
        else if (word.length() == 0) {
            available_completions.clear();
        }
        
        completed_word = word;
        completion_pos = int(word.length());
    }



    /**
     * @brief Does identifier qualify as boolean?
     * 
     * If this is an IDENTIFIER completion, the identifier is of integer type
     * and doesn't have the flag HEX, check if minvalue is 0 and maxvalue is 1
     * 
     * @return true if identifier min value is 0 and max value is 1
     */
    bool Completion::is_ident_bool()
    {
        if (type != IDENTIFIER) {
            return false;
        }

        if (id.type == ID_VAR && !(id.flags & IDF_HEX))
        {
            if (id.minval == 0 && id.maxval == 1) {
                return true;
            }
        }
        return false;
    }



    /**
     * @brief puts min, max, default and current value of identifier into array std::string[4]
     * 
     * Puts Min, Max, Default and Current value, converted to std::string into array.
     * values[0] = min value
     * values[1] = max value
     * values[2] = default value
     * values[3] = current value
     * 
     * If identifier is of type string or qualifies as bool,only default and current value will be put into the list
     * If min or max value are equal to VAR_MIN/VAR_MAX/FLOAT_MIN/FLOAT_MAX, 
     * they will be set to INT MIN/INT MAX/FLOAT MIN/FLOAT MAX
     * 
     * @param std::string (&values)[4], array where values will be put into
     */
    void Completion::get_ident_values(std::string (&values)[4])
    {
        if (type != IDENTIFIER) {
            return;
        }

        switch (id.type)
        {
            case ID_VAR:
                if (id.flags & IDF_HEX)
                {
                    values[0] = int_to_hex(id.minval);
                    values[1] = int_to_hex(id.maxval);
                    values[2] = int_to_hex(id.def.i);
                    values[3] = int_to_hex(*id.storage.i);
                }
                else if (is_ident_bool())
                {
                    values[2] = std::to_string(id.def.i) + (id.def.i == 1 ? " (true)" : " (false)");
                    values[3] = std::to_string(*id.storage.i) + (*id.storage.i == 1 ? " (true)" : " (false)");
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
                break;
            case ID_SVAR:
                values[2] = id.def.s;
                values[3] = *id.storage.s;
                break;
        }
    }



    /**
     * @brief Get identifier flags listed as std::string e.g. "persistent, client"
     * @return identifier flags listed as std::string
     */
    std::string Completion::get_ident_flags_text()
    {
        if (type != IDENTIFIER) {
            return "";
        }

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
            // cut away ", " at the end
            flags = flags.substr(0, flags.length() - 2);
        }
        return flags;
    }



    /**
     * @brief Get identifier args listed as std::string e.g. "<int/bool> <int> <string>"
     * @return identifier args listed as std::string
     */
    std::string Completion::get_ident_args_text()
    {
        std::string args = "";
        switch (id.type)
        {
            case ID_ALIAS: args += " <args>"; break;
            case ID_VAR:
                if (is_ident_bool()) { args += " <int/bool>"; }
                else                 { args += " <int>";      }
                break;
            case ID_FVAR:  args += " <float>"; break;
            case ID_SVAR:  args += " <string>"; break;
            case ID_COMMAND:
                if (id.args != nullptr)
                {
                    for (int i = 0; i < int(strlen(id.args)); ++i)
                    {
                        switch (id.args[i])
                        {
                            case 's': args += " <string>"; break;
                            case 'i': case 'b': case 'N':
                                args += std::string(" <") 
                                       + (id.flags & IDF_HEX ? "bitfield" : "int") 
                                       + std::string(">");
                                break;
                            case 'f':
                            case 'g': args += " <float>";     break;
                            case 't': args += " <null>";      break;
                            case 'e': args += " <commands>";  break;
                            case 'r': case '$': args += " <ident>";     break;
                            default: args += " <?>";          break;
                        }
                    }
                }
                break;
        }
        return args;
    }
};
