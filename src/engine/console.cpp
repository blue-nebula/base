// console.cpp: the console buffer, its display, and command line control
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>

#include "console.h"
#include "console_completion.h"
#include "engine.h"
#include "game.h"

VAR(IDF_PERSIST|IDF_HEX, saytextcolour, -1, 0xFFFFFF, 0xFFFFFF);
VAR(IDF_PERSIST, enterclosesconsole, 0, 1, 1);
VAR(IDF_PERSIST, consolesyntaxhighlight, 0, 0, 1);
TVAR(IDF_PERSIST|IDF_GAMEPRELOAD, console_command_tex, "textures/icons/fontawesome/command", 3);
TVAR(IDF_PERSIST|IDF_GAMEPRELOAD, console_chat_tex, "textures/chat", 3);

std::string unescapestring(std::string src)
{
    // go through and replace every ^n, ^f and ^t with \n, \f and \t   
    int len = int(src.length());
    std::string dst = "";
    for (int i = 0; i < len; i++)
    {
        char c = src[i];
        char next_c = i + 1 < len ? src[i + 1] : ' ';

        if (c == '^')
        {
            switch (next_c)
            {
                case 'n':
                    dst += '\n';
                    i++;
                    continue;
                case 't':
                    dst += '\t';
                    i++;
                    continue;
                case 'f':
                    dst += '\f';
                    i++;
                    continue;
            }
        }
        dst += c;
    }

    return dst;
}

int ConsoleLine::get_num_lines()
{
    return int(lines.size());
}

bool InputHistory::scroll(const int lines)
{
    if (lines == 0 || int(history.size()) == 0)
    {
        return false;
    }
    
    const int prev_hist_pos = hist_pos;
        
    if (lines > 0)
    {
        const int max_pos = int(history.size()) - 1;
        hist_pos = std::min(hist_pos + lines, max_pos);
    }
    else
    {
        hist_pos = std::max(hist_pos + lines, 0);
    }

    hist_pos = std::max(hist_pos, 0);

    current_line = history[hist_pos];
    return hist_pos != prev_hist_pos;
}

void InputHistory::save(InputHistoryLine line)
{
    // if there are any entries inside history, check if the last one is basically
    // the same as the current one, because if so, we don't need to save it another time
    if (int(history.size()) > 0)
    {
        InputHistoryLine& prev_line = history.front();
        if (   line.text == prev_line.text
            && line.icon == prev_line.icon
            && line.action == prev_line.action)
        {
            return;
        }
    }

    history.push_front(line);
}

int History::get_num_lines()
{
    return num_linebreaks;
}

int History::get_num_unseen_messages()
{
    return unseen_messages;
}

std::array<int, 2> History::get_scroll_info()
{
    return std::array<int, 2>{ scroll_info_hist_idx, scroll_info_line_idx };
}

void History::set_line_width(int w)
{
    int prev_line_width = line_width;

    line_width = w;

    if (line_width != prev_line_width)
    {
        calculate_all_wordwraps();
    }
}

int History::get_scroll_pos()
{
    return scroll_pos;
}

void History::reset_scroll()
{
    scroll_pos = 0;
    recalc_scroll_info();
}

bool History::scroll(const int lines)
{
    if (lines == 0)
    {
        return false;
    }

    const int prev_scroll_pos = scroll_pos;

    if (lines > 0)
    {
        // go up
        //TODO: Find alternative to using idents hashnameset
        const int max_pos = num_linebreaks - new_console.get_page_size();
        scroll_pos = std::min(scroll_pos + lines, max_pos);
    }
    else
    {
        // lines is smaller than 0, go down
        scroll_pos = std::max(scroll_pos + lines, 0);
    }

    // make sure scrollpos never gets below 0
    scroll_pos = std::max(scroll_pos, 0);

    bool scrolled = scroll_pos != prev_scroll_pos;
    if (scrolled)
    {
        recalc_scroll_info();
    
        // recalc the number of unseen messages
        unseen_messages = 0; 
        for (int i = 0; i < scroll_info_hist_idx; i++)
        {
            if (h[i].seen == false)
            {
                unseen_messages++;
            }
        }
    }

    return scrolled;
}

void History::remove(const int idx)
{
    num_linebreaks -= h[idx].get_num_lines();
    h.erase(h.begin() + idx);
}

void History::recalc_scroll_info()
{
    int linebreaks_iterated = 0;
   
    for (int i = 0; i < int(h.size()); i++)
    {
        int line_count = int(h[i].lines.size());
        if ((linebreaks_iterated + line_count) > scroll_pos)
        {
            for (int j = line_count - 1; j >= 0; j--)
            {
                if (linebreaks_iterated == scroll_pos)
                {
                    scroll_info_hist_idx = i;
                    scroll_info_line_idx = j;
                    scroll_info_outdated = false;
                    return;
                }

                linebreaks_iterated++;
            }
        }
        else
        {
            linebreaks_iterated += line_count;
        }
    }
    printf("wtf, couldn't find shit\n");
}

// returns line info relative to given hist_idx and line_idx
// format: std::pair<hist_idx, line_idx>
std::pair<int, int> History::get_relative_line_info(int n, int hist_idx, int line_idx)
{
    if (scroll_info_outdated)
    {
        recalc_scroll_info();
    }

    if (line_idx == -1)
    {
        line_idx = h[hist_idx].lines.size() - 1;
    }

    for (int i = 0; i < n; i++)
    {
        if (line_idx <= 0)
        {
            hist_idx++;
        }
        line_idx--;

        if (line_idx == -1)
        {
            line_idx = h[hist_idx].lines.size() - 1;
        }
    }
    
    return std::make_pair(hist_idx, line_idx);
}

void History::set_max_entries(const int entries)
{
    max_num_entries = entries;
}

void History::save(ConsoleLine& line)
{
    calculate_wordwrap(line);
    num_linebreaks += line.get_num_lines();
    //TODO: call remove instead of doing it the same way again
    if (int(h.size()) > max_num_entries)
    {
        printf("Remove shit\n");
        num_linebreaks -= h.back().get_num_lines();
        h.pop_back();
    }

    h.push_front(line);

    if (scroll_pos > 0)
    {
        h[0].seen = false;
        missed_lines++;
    
        // scroll up by h[0].get_num_lines() to account for the lines shifting when
        // adding the new lines
        scroll(h[0].get_num_lines());
    }

    // set scroll info outdated to true, it will be recalculated
    // as soon the history is shown
    scroll_info_outdated = true;
}

void History::clear()
{
    h.clear();
    num_linebreaks = 0;
    scroll_pos = 0;
    scroll_info_hist_idx = 0;
    scroll_info_line_idx = 0;
    scroll_info_outdated = false;
}

void History::calculate_wordwrap(ConsoleLine& line)
{
    line.lines.clear();

    if (curfont != nullptr)
    {
        // works best when using fontsize default
        pushfont("default");
        std::vector<std::pair<int, std::string>> wraps = get_text_wraps(line.text.c_str(), line_width);
        popfont();

        int prev_pos = 0;
        for (const auto& wrap : wraps)
        {
            std::string text = wrap.second.empty() ? "" : "\f" + wrap.second; 
            text += line.text.substr(prev_pos, wrap.first - prev_pos);
            // remove every occurence of \n from text, as it has already been accounted for in get_text_wraps
            text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
            line.lines.push_back(text);
            prev_pos += wrap.first - prev_pos;
        }
    } 
    
    if (int(line.lines.size()) == 0)
    {
        line.lines.push_back(line.text);
    }
}

void History::calculate_all_wordwraps()
{
    num_linebreaks = 0;
    //TODO: measure time
    for (int i = 0; i < int(h.size()); i++)
    {
        ConsoleLine line = h[i];
        calculate_wordwrap(line);
        num_linebreaks += line.get_num_lines();
        h[i] = line;
    }
}

Console::Console()
{
    histories[HIST_CHAT] = History();
    histories[HIST_CONSOLE] = History();
    histories[HIST_PREVIEW] = History();

    histories[HIST_CHAT].set_max_entries(1000);
    histories[HIST_CONSOLE].set_max_entries(1000);
    histories[HIST_PREVIEW].set_max_entries(7);

    //TODO: make preview_history max size dynamic
    
    histories[HIST_CHAT].type_background_colors[CON_CHAT_WHISPER] = {.7f, .7f, .7f, .2f};
    histories[HIST_CHAT].type_background_colors[CON_CHAT_TEAM] = {0, 0, .1f, .8f};

    histories[HIST_CONSOLE].type_background_colors[CON_DEBUG_ERROR] = { .8f, .1f, .1f, .8f };

    // in milliseconds, < 0 means use default (defined in hud.cpp)
    /*                                 fade in | wait | fade out            */
    type_fade_times[CON_CHAT] =         std::array<short, 3>{ -1, 5000, -1 };
    type_fade_times[CON_CHAT_TEAM] =    std::array<short, 3>{ -1, 5000, -1 };
    type_fade_times[CON_CHAT_WHISPER] = std::array<short, 3>{ -1, 5000, -1 };
    type_fade_times[CON_GAME] =         std::array<short, 3>{ -1, 2000, -1 };
    type_fade_times[CON_INFO] =         std::array<short, 3>{ -1, 2000, -1 };
    type_fade_times[CON_FRAG] =         std::array<short, 3>{ -1, 3000, -1 };
    type_fade_times[CON_SELF] =         std::array<short, 3>{ -1, 4000, -1 };
    type_fade_times[CON_GAME_INFO] =    std::array<short, 3>{ -1, 9000, -1 };
    

    register_completion(new PlayerNameCompletion());
    register_completion(new MapNameCompletion());
    register_completion(new CommandCompletion());
}

const int Console::get_page_size()
{
    return (*idents["consize"].storage.i + *idents["conoverflow"].storage.i);
}

int Console::get_say_text_color()
{
    if (saytextcolour < 0)
    {
        return game::getcolour(&game::player1, 1);
    }
    return saytextcolour;
}

History& Console::curr_hist()
{
    if (selected_hist < HIST_MAX && selected_hist >= 0)
    {
        return histories[selected_hist];
    }
    // fallback
    return histories[HIST_CHAT];
}

// is called whenever buffer changes
void Console::set_buffer(std::string text)
{
    std::string buffer_before = buffer;
    buffer = text;
    if (buffer != input_history.current_line.text && input_history.hist_pos != -1)
    {
        // reset the InputHistory position because now we have changed it
        // otherwise you wouldn't be able to get back to the text you changed before
        input_history.hist_pos = -1;
    }

    // check for any completions
    if (buffer_before != buffer)
    {
        const size_t prev_length = curr_completions.size();
        curr_completions.clear();
        curr_completion_engine = -1;
        int i = 0;
        for (auto* completion_engine : completions_engines)
        {
            if (completion_engine->can_complete(*this))
            {
                curr_completions = completion_engine->get_completions(this->buffer);
                if (int(curr_completions.size()) > 0)
                {
                    curr_completion_engine = i;
                    break;
                }
            }
            i++;
        }
        if (prev_length != curr_completions.size())
        {
            completion_scroll_pos = 0;
            completion_selection_idx = 0;
        }
    }
}

void Console::see_line(ConsoleLine& line)
{
    if (!line.seen)
    {
        line.seen = true;
        
        if (line.type == CON_DEBUG_ERROR)
        {
            new_console.unseen_error_messages--;
        }
    }
}

bool Console::is_open()
{
    return open;
}

void Console::set_max_line_width(int w)
{
    for (int i = 0; i < HIST_MAX; i++)
    {
        histories[i].set_line_width(w);
    }
}

std::string Console::get_buffer()
{
    return buffer;
}
ICOMMAND(0, getconsolebuffer, "", (), stringret(newstring(new_console.get_buffer().c_str())));

int Console::get_cursor_pos()
{
    return cursor_pos == -1 ? int(get_buffer().length()) : cursor_pos;
}

int Console::get_completion_scroll_pos()
{
    return completion_scroll_pos;
}

int Console::get_completion_selection()
{
    return completion_selection_idx;
}

int Console::get_completion_lines_per_view()
{
    return completion_lines_per_view;
}

void Console::print(int type, const std::string text)
{
    if (text.empty())
    {
        printf("Error printing empty text of type %d\n", type);
        return;
    }

    ConsoleLine line = ConsoleLine();
    line.text = text;
    line.type = type;
    line.reftime = totalmillis;
    line.out_time = totalmillis;
    line.real_time = clocktime;

    // filter
    switch (line.type)
    {
        case CON_CHAT:
        case CON_CHAT_TEAM:
        case CON_CHAT_WHISPER:
        case CON_SELF:
        case CON_GAME_INFO:
            histories[HIST_CHAT].save(line);
            break;
        case CON_DEBUG_ERROR: /* FALLTHROUGH */
            unseen_error_messages++;
        default:
            histories[HIST_CONSOLE].save(line);
            break;
    }

    switch (line.type)
    {
        case CON_CHAT:
        case CON_CHAT_TEAM:
        case CON_CHAT_WHISPER:
        case CON_INFO:
        case CON_SELF:
        case CON_FRAG:
        case CON_GAME:
        case CON_GAME_INFO:
            histories[HIST_PREVIEW].save(line);
            break;
    }
}
// this is called when pressing enter
void Console::run_buffer()
{
    // you can't send empty messages (checks if there are any non-space characters)
    if (buffer.find_first_not_of(' ') != std::string::npos)
    {
        if (buffer[0] == command_prefix)
        {
            // the + 1 is to remove the command_prefix from the beginning of the buffer
            execute(buffer.c_str() + 1);
        }
        else
        {   
            client::toserver(curr_action, unescapestring(buffer).c_str()); 
            //client::toserver(0, unescapestring(buffer).c_str());
            //execute(curr_action.c_str());
        }

        // reset scrolling
        curr_hist().reset_scroll();
    }
}

void Console::open_console()
{    
    textinput(true, TI_CONSOLE);
    keyrepeat(true, KR_CONSOLE);

    open = true;
}

void Console::close_console()
{
    textinput(false, TI_CONSOLE);
    keyrepeat(false, KR_CONSOLE);

    open = false;
    set_buffer("");
    curr_action = 0;
    curr_icon = "";
    input_history.hist_pos = -1;
}

void Console::insert_in_buffer(const std::string text)
{
    // client::maxmsglen() - "^f[0x000000]".length(), because
    // when sending we will append the saytextcolour to the beginning,
    // if we don't account for that the message will be cut short
    int maxlen = std::min(client::maxmsglen() - 13, max_buffer_len);

    int remaining_space = maxlen - int(buffer.length());
    int insert_len = std::min(int(text.length()), remaining_space);

    if (insert_len <= 0)
    {
        // can't insert anything
        return;
    }

    if (cursor_pos < 0)
    {
        set_buffer(buffer + text);
    }
    else
    {
        std::string new_buffer = get_buffer();
        new_buffer.insert(std::max(0, cursor_pos), text);
        set_buffer(new_buffer);
    }

    for (int i = 0; i < insert_len; i++)
    {
        cursor_move_right();
    }
}

int Console::get_mode()
{
    if (!buffer.empty())
    {
        if (buffer[0] == command_prefix)
        {
            return MODE_COMMAND;
        }
    }
    
    return MODE_NONE;
}

std::string Console::get_info_bar_text()
{
    if (curr_hist().get_num_unseen_messages() > 0)
    {
        static const std::string UNSEEN_MESSAGES_COLOR = "\fzyw";
        static const std::string UNSEEN_MESSAGES = " Unseen Messages";

        return UNSEEN_MESSAGES_COLOR + std::to_string(curr_hist().get_num_unseen_messages()) + UNSEEN_MESSAGES;
    }
    return "";
}

Console new_console = Console();

reversequeue<cline, MAXCONLINES> conlines;

enum { CF_COMPLETE = 1<<0, CF_EXECUTE = 1<<1, CF_MESSAGE = 1<<2 };
int commandcolour = 0;

void complete(char* s, size_t s_size, const char* cmdprefix);

void conline(int type, const char *sf, int n)
{
    if (sf && *sf)
    {
        new_console.print(type, sf);
    }
    else
    {
        printf("Excuse me wtf\n");
    }
}

void inputcommand(char *init, int action = 0, char *icon = NULL, int colour = 0, char *flags = NULL) // turns input to the command line on or off
{
    new_console.set_input(init != nullptr ? init : "", 
                        action, 
                        icon != nullptr ? icon : "");
    /*
    commandmillis = init ? totalmillis : -totalmillis;
    textinput(commandmillis >= 0, TI_CONSOLE);
    keyrepeat(commandmillis >= 0, KR_CONSOLE);
    if (init != nullptr)
    {
        new_console.set_buffer(init);
    }

    //DELETEA(commandaction);
    //DELETEA(commandicon);
    new_console.cursor_pos = -1;
    if(action && action[0]) commandaction = newstring(action);
    if(icon && icon[0]) commandicon = newstring(icon);
    commandcolour = colour;
    commandflags = 0;
    if(flags) while(*flags) switch(*flags++)
    {
        case 'c': commandflags |= CF_COMPLETE; break;
        case 'x': commandflags |= CF_EXECUTE; break;
        case 'm': commandflags |= CF_MESSAGE; break;
        case 's': commandflags |= CF_COMPLETE|CF_EXECUTE|CF_MESSAGE; break;
    }
    else if(init) commandflags |= CF_COMPLETE|CF_EXECUTE;
    */
}

void Console::set_input(std::string init, int action, std::string icon)
{
    open_console();

    set_buffer(init);

    new_console.cursor_pos = -1;
    curr_action = action;
    curr_icon = icon;
}

ICOMMAND(0, saycommand, "C", (char *init), inputcommand(init, SAY_NONE));
ICOMMAND(0, inputcommand, "sisis", (char *init, int* action, char *icon, int *colour, char *flags), inputcommand(init, *action, icon, *colour, flags));
ICOMMAND(0, saytextcommand, "C", (char* init), inputcommand(init, SAY_NONE));
ICOMMAND(0, sayteamcommand, "C", (char* init), inputcommand(init, SAY_TEAM));

bool paste(char *buf, size_t len)
{
    if(!SDL_HasClipboardText()) return false;
    char *cb = SDL_GetClipboardText();
    if(!cb) return false;
    size_t cblen = strlen(cb),
           start = strlen(buf),
           decoded = decodeutf8((uchar *)&buf[start], len-1-start, (const uchar *)cb, cblen);
    buf[start + decoded] = '\0';
    SDL_free(cb);
    return true;
}

bool paste_to_buffer()
{
    if (!SDL_HasClipboardText())
    {
        return false;
    }

    char* cb = SDL_GetClipboardText();
    if (cb == nullptr)
    {
        return false;
    }
    size_t len = new_console.max_buffer_len;
    size_t cblen = strlen(cb);
    char* buf = newstring("");
    size_t start = strlen(buf);
    
    decodeutf8((uchar *)&buf[start], len - 1 - start, (const uchar *)cb, cblen);
   

    new_console.insert_in_buffer(buf);

    // documentation says this has to be done
    SDL_free(cb);

    return true;
}

int histpos = 0;

VAR(IDF_PERSIST, maxhistory, 0, 1000, 10000);
/*
void history_(int *n)
{
    static bool inhistory = false;
    if(!inhistory && history.inrange(*n))
    {
        inhistory = true;
        history[history.length()-*n-1]->run();
        inhistory = false;
    }
}
*/

//COMMANDN(0, history, history_, "i");

bool Console::process_text_input(const char *str, int len)
{
    if (!open)
    {
        return false;
    }

    //resetcomplete();
    new_console.insert_in_buffer(str);

    return true;
}

bool Console::process_key(int code, bool isdown)
{
    if (!open)
    {
        return false;
    }

    if (isdown)
    {
        switch (code)
        {
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                break;

            case SDLK_HOME:
                cursor_jump_to_buffer_start(); 
                break;

            case SDLK_END:
                cursor_jump_to_buffer_end();
                break;

            case SDLK_DELETE:
                buffer_delete_at_cursor();
                break;

            case SDLK_BACKSPACE:
                buffer_remove_at_cursor();
                break;

            case -4:
                if (SDL_GetModState() & KMOD_SHIFT)
                {
                    curr_hist().scroll(10);
                    break;
                }
                curr_hist().scroll(1);
                break;

            case -5:
                if (SDL_GetModState() & KMOD_SHIFT)
                {
                    curr_hist().scroll(-10);
                }
                curr_hist().scroll(-1);
                break;

            case SDLK_PAGEUP:
                curr_hist().scroll(get_page_size());
                break;

            case SDLK_PAGEDOWN:
                curr_hist().scroll(-get_page_size());
                break;

            case SDLK_LEFT:
                cursor_move_left();
                break;

            case SDLK_RIGHT:
                cursor_move_right();
                break;

            case SDLK_UP: 
                if (int(curr_completions.size()) != 0)
                {
                    completion_scroll(-1);
                    break;
                }

                if (input_history.scroll(1))
                {
                    set_buffer(input_history.current_line.text);
                    curr_icon = input_history.current_line.icon;
                    curr_action = input_history.current_line.action;
                }
                break;

            case SDLK_DOWN:
                if (int(curr_completions.size()) != 0)
                {
                    completion_scroll(1);
                    break;
                }
                
                if (input_history.scroll(-1))
                {
                    set_buffer(input_history.current_line.text);
                    curr_icon = input_history.current_line.icon;
                    curr_action = input_history.current_line.action;
                }
                break;
            
            case SDLK_v:
                if (SDL_GetModState() & MOD_KEYS)
                {
                    paste_to_buffer();
                }
                break;

            case SDLK_F1:
                selected_hist = HIST_CHAT;
                break;

            case SDLK_F2:
                selected_hist = HIST_CONSOLE;
                break;

            case SDLK_TAB:
                if (SDL_GetModState() & KMOD_CTRL)
                {
                    // switch between tabs using "TAB" 
                    if (selected_hist < HIST_MAX - 1)
                    {
                        selected_hist++;
                    }
                    else
                    {
                        selected_hist = 0;
                    } 
                    break;
                }

                if (int(curr_completions.size()) != 0 
                    && completion_scroll_pos != -1
                    && curr_completion_engine != -1)
                {
                    completions_engines[curr_completion_engine]->select_entry(curr_completions[completion_selection_idx], *this);
                }
                break;
        }
    }
    else
    {
        if (code == SDLK_RETURN || code == SDLK_KP_ENTER)
        { 
            if (!get_buffer().empty())
            {
                // save this line to the history
                InputHistoryLine line = InputHistoryLine();
                line.text = get_buffer();
                line.action = curr_action;
                line.icon = curr_icon;
                input_history.save(line);
            }
             
            interactive = true;
            run_buffer();
            interactive = false;
            
            if (enterclosesconsole)
            {
                close_console();
            }
        }
        else if (code == SDLK_ESCAPE || (code < 0 && code != -5 && code != -4))
        {
            close_console();
        }
    }

    return true;
}

std::string Console::get_icon()
{
    switch (get_mode())
    {
        case MODE_NONE:
            switch (curr_action)
            {
                case SAY_NONE:
                    return console_chat_tex;
                case SAY_TEAM:
                    return hud::teamtexname(game::player1.team);
            }
            break;
        case MODE_COMMAND:
            return console_command_tex;
    }
    return curr_icon;
}

int Console::get_icon_color()
{
    if (get_mode() == MODE_NONE)
    {
        switch (curr_action)
        {
            case SAY_NONE:
                return 0xFFFFFF;
            case SAY_TEAM:
                return TEAM(game::player1.team, colour);
        }
    }

    return 0xFFFFFF;
}

void Console::clear_curr_hist()
{
    curr_hist().clear();
}
ICOMMAND(0, clear, "", (), new_console.clear_curr_hist());


/// COMPLETION ///
//////////////////

bool Console::completion_scroll(const int lines)
{
    if (lines == 0
        || int(curr_completions.size()) == 0)
    {
        return false;
    }

    static const int max_dist = 2;

    if (lines < 0)
    {
        int scroll_pos = completion_scroll_pos;
        int selection = completion_selection_idx;

        // scroll up selection
        selection = std::max(selection - 1, 0);

        // check if scroll_pos needs to be scrolled too
        if ((selection - scroll_pos) <= max_dist - 1)
        {
            scroll_pos = std::max(scroll_pos - 1, 0);
        }

        completion_scroll_pos = scroll_pos;
        completion_selection_idx = selection;
    }
    else
    {
        int scroll_pos = completion_scroll_pos;
        int selection = completion_selection_idx;
        // first move down selection
        const int max_pos = int(curr_completions.size());
        selection = std::min(selection + lines, max_pos - 1);

        // check if scroll_pos needs to be moved down too
        if ((selection - scroll_pos) >= completion_lines_per_view - max_dist)
        {
            scroll_pos = std::min(scroll_pos + 1, max_pos - completion_lines_per_view);
        }

        completion_scroll_pos = scroll_pos;
        completion_selection_idx = selection;
    }
    return false;
}

void Console::register_completion(CompletionBase* completion)
{
    completions_engines.push_back(completion);
}

std::vector<CompletionEntryBase*> Console::get_curr_completions()
{
    return curr_completions;
}

///////////////////
/// KEY ACTIONS ///
///////////////////


void Console::cursor_jump_to_buffer_start()
{
    if (get_buffer().length() != 0)
    {
        cursor_pos = 0;
    }
}

void Console::cursor_jump_to_buffer_end()
{
    cursor_pos = -1;
}

void Console::cursor_move_left()
{
    if (cursor_pos > 0)
    {
        cursor_pos--;
    }
    else if (cursor_pos == -1)
    {
        cursor_pos = int(get_buffer().length()) - 1;
    }
}

void Console::cursor_move_right()
{
    if (cursor_pos >= int(get_buffer().length()) - 1)
    {
        cursor_pos = -1;
    }
    else if (cursor_pos >= 0)
    {
        cursor_pos = std::min(cursor_pos + 1, int(get_buffer().length()) - 1);
    }
}

void Console::buffer_delete_at_cursor()
{
    if (cursor_pos == -1 || int(get_buffer().length()) == 0)
    {
        return;
    }

    std::string new_buffer = get_buffer();
    new_buffer.erase(new_buffer.begin() + cursor_pos);
    set_buffer(new_buffer);

    if (cursor_pos > int(get_buffer().length()) - 1)
    {
        cursor_pos = -1;
    }
}

void Console::buffer_remove_at_cursor()
{
    // if the cursor at -1 or 0, there is no text to remove
    /*if (cursor_pos <= 0)
    {
        return;
    }*/

    int len = int(get_buffer().length());
    int i = cursor_pos >= 0 ? cursor_pos : len;

    if (i < 1)
    {
        return;
    }

    std::string new_buffer = get_buffer();
    new_buffer.erase(new_buffer.begin() + (i - 1));
    set_buffer(new_buffer);
                
    if (cursor_pos > 0)
    {
        cursor_pos--;
    }
    else if (cursor_pos == 0 && len <= 1)
    {
        cursor_pos = -1;
    }
}

void writebinds(stream *f)
{
    std::string binds = input_system.get_save_binds_string();
    f->printf(binds.c_str());
} 

// tab-completion of all idents and base maps

enum { FILES_DIR = 0, FILES_LIST };

struct fileskey
{
    int type;
    const char *dir, *ext;

    fileskey() {}
    fileskey(int type, const char *dir, const char *ext) : type(type), dir(dir), ext(ext) {}
};

struct filesval
{
    int type;
    char *dir, *ext;
    vector<char *> files;
    int millis;

    filesval(int type, const char *dir, const char *ext) : type(type), dir(newstring(dir)), ext(ext && ext[0] ? newstring(ext) : NULL), millis(-1) {}
    ~filesval() { DELETEA(dir); DELETEA(ext); loopv(files) DELETEA(files[i]); files.shrink(0); }

    void update()
    {
        if(type!=FILES_DIR) return;
        files.deletearrays();
        listfiles(dir, ext, files);
        files.sort();
        loopv(files) if(i && !strcmp(files[i], files[i-1])) delete[] files.remove(i--);
        millis = totalmillis;
    }
};

static inline bool htcmp(const fileskey &x, const fileskey &y)
{
    return x.type==y.type && !strcmp(x.dir, y.dir) && (x.ext == y.ext || (x.ext && y.ext && !strcmp(x.ext, y.ext)));
}

static inline uint hthash(const fileskey &k)
{
    return hthash(k.dir);
}

static hashtable<fileskey, filesval *> completefiles;
static hashtable<char *, filesval *> completions;

int completeoffset = -1, completesize = 0;
bigstring lastcomplete;

void resetcomplete() { completesize = 0; }

void addcomplete(char *command, int type, char *dir, char *ext)
{
    if(identflags&IDF_WORLD)
    {
        conoutf("\frcannot override complete %s", command);
        return;
    }
    if(!dir[0])
    {
        filesval **hasfiles = completions.access(command);
        if(hasfiles) *hasfiles = NULL;
        return;
    }
    if(type==FILES_DIR)
    {
        int dirlen = (int)strlen(dir);
        while(dirlen > 0 && (dir[dirlen-1] == '/' || dir[dirlen-1] == '\\')) dir[--dirlen] = '\0';
        if(ext)
        {
            if(strchr(ext, '*')) ext[0] = '\0';
            if(!ext[0]) ext = NULL;
        }
    }
    fileskey key(type, dir, ext);
    filesval **val = completefiles.access(key);
    if(!val)
    {
        filesval *f = new filesval(type, dir, ext);
        if(type==FILES_LIST) explodelist(dir, f->files);
        val = &completefiles[fileskey(type, f->dir, f->ext)];
        *val = f;
    }
    filesval **hasfiles = completions.access(command);
    if(hasfiles) *hasfiles = *val;
    else completions[newstring(command)] = *val;
}

void addfilecomplete(char *command, char *dir, char *ext)
{
    addcomplete(command, FILES_DIR, dir, ext);
}

void addlistcomplete(char *command, char *list)
{
    addcomplete(command, FILES_LIST, list, NULL);
}

COMMANDN(0, complete, addfilecomplete, "sss");
COMMANDN(0, listcomplete, addlistcomplete, "ss");

void complete(char *s, size_t s_size, const char *cmdprefix)
{
    char* name = strrchr(s, '@');

    if (name)
    {
        // remove the @ at the start
        name++;

        int name_len = strlen(name);

        // loop through all players
        for (auto i = 0; i < game::players.length(); i++)
        {
            // exclude bots
            if (!game::players[i] || (game::players[i]->actortype == ENT_AI)) 
            {
                continue;
            }
            // check if the playername fits the input
            bool fits = strncmp(name, game::players[i]->name, name_len) == 0;

            // replace the input with the playername
            if (fits) 
            {
                std::string formatted_playername = game::colourname(game::players[i]);

                // remove everything to the @ from s
                int s_len = strlen(s);
                s[s_len - name_len - 1] = 0;

                // add player_name to s and return
                strncat(s, formatted_playername.c_str(), s_size - s_len - name_len - 1 - formatted_playername.length());
                return;
            }
        }
        return;
    }
    char *start = s;

    if(cmdprefix)
    {
        int cmdlen = strlen(cmdprefix);
        if(strncmp(s, cmdprefix, cmdlen)) prependstring(s, cmdprefix, s_size);
        start = &s[cmdlen];
    }
    

    const char chrlist[7] = { ';', '(', ')', '[', ']', '\"', '$', };
    bool variable = false;
    loopi(7)
    {
        char *semi = strrchr(start, chrlist[i]);
        if(semi)
        {
            start = semi+1;
            if(chrlist[i] == '$') variable = true;
        }
    }
    while(*start == ' ') start++;
    if(!start[0]) return;
    if(start-s != completeoffset || !completesize)
    {
        completeoffset = start-s;
        completesize = (int)strlen(start);
        lastcomplete[0] = '\0';
    }
    filesval *f = NULL;
    if(completesize)
    {
        char *end = strchr(start, ' ');
        if(end) f = completions.find(stringslice(start, end), NULL);
    }
    const char *nextcomplete = NULL;
    int prefixlen = start-s;
    if(f) // complete using filenames
    {
        int commandsize = strchr(start, ' ')+1-start;
        prefixlen += commandsize;
        f->update();
        loopv(f->files)
        {
            if(strncmp(f->files[i], &start[commandsize], completesize-commandsize)==0 &&
                strcmp(f->files[i], lastcomplete) > 0 && (!nextcomplete || strcmp(f->files[i], nextcomplete) < 0))
                nextcomplete = f->files[i];
        }
    }
    else // complete using command names
    {
        enumerate(idents, ident, id,
            if((variable ? id.type == ID_VAR || id.type == ID_SVAR || id.type == ID_FVAR || id.type == ID_ALIAS: id.flags&IDF_COMPLETE) && strncmp(id.name, start, completesize)==0 &&
            strcmp(id.name, lastcomplete) > 0 && (!nextcomplete || strcmp(id.name, nextcomplete) < 0))
                nextcomplete = id.name;
        );
    }
    if(nextcomplete)
    {
        copystring(&s[prefixlen], nextcomplete, s_size-prefixlen);
        copystring(lastcomplete, nextcomplete, s_size);
    }
    else
    {
        if((int)strlen(start) > completesize) start[completesize] = '\0';
        completesize = 0;
    }
}



void setidflag(const char *s, const char *v, int flag, const char *msg, bool alias)
{
    ident *id = idents.access(s);
    if(!id || (alias && id->type != ID_ALIAS))
    {
        if(verbose) conoutf("\fradding %s of %s failed as it is not available", msg, s);
        return;
    }
    bool on = false;
    if(isnumeric(*v)) on = atoi(v) != 0;
    else if(!strcasecmp("false", v)) on = false;
    else if(!strcasecmp("true", v)) on = true;
    else if(!strcasecmp("on", v)) on = true;
    else if(!strcasecmp("off", v)) on = false;
    if(on && !(id->flags&flag)) id->flags |= flag;
    else if(!on && id->flags&flag) id->flags &= ~flag;
}

ICOMMAND(0, setcomplete, "ss", (char *s, char *t), setidflag(s, t, IDF_COMPLETE, "complete", false));
ICOMMAND(0, setpersist, "ss", (char *s, char *t), setidflag(s, t, IDF_PERSIST, "persist", true));

void setiddesc(const char *s, const char *v, const char *f)
{
    ident *id = idents.access(s);
    if(!id)
    {
        if(verbose) conoutf("\fradding description of %s failed as it is not available", s);
        return;
    }
    DELETEA(id->desc);
    if(v && *v) id->desc = newstring(v);
    loopvrev(id->fields)
    {
        DELETEA(id->fields[i]);
        id->fields.remove(i);
    }
    if(f && *f) explodelist(f, id->fields);
}
ICOMMAND(0, setdesc, "sss", (char *s, char *t, char *f), setiddesc(s, t, f));

void writecompletions(stream *f)
{
    vector<char *> cmds;
    enumeratekt(completions, char *, k, filesval *, v, { if(v) cmds.add(k); });
    cmds.sort();
    loopv(cmds)
    {
        char *k = cmds[i];
        filesval *v = completions[k];
        if(v->type==FILES_LIST)
        {
            if(validateblock(v->dir)) f->printf("    listcomplete %s [%s]\n", escapeid(k), v->dir);
            else f->printf("    listcomplete %s %s\n", escapeid(k), escapestring(v->dir));
        }
        else f->printf("    complete %s %s %s\n", escapeid(k), escapestring(v->dir), escapestring(v->ext ? v->ext : "*"));
    }
}


#ifndef STANDALONE
bool consolegui(guient *g, int width, int height, const char *init, int &update)
{
    g->strut(width-6);
    if(!conlines.empty() && (update < 0 || conlines[0].reftime > update))
    {
        editor *e = UI::geteditor("console_window", EDITORREADONLY);
        if(e)
        {
            UI::editorclear(e);
            loopvrev(conlines) UI::editorline(e, conlines[i].cref, MAXCONLINES);
            update = totalmillis;
        }
    }
    g->field("console_window", 0x666666, -width, height, NULL, EDITORREADONLY);
    char *w = g->field("console_input", 0x666666, -width, 0, init, EDITORFOREVER, g->visible(), "console_window");
    if(w && *w)
    {
        bool consolecmd = *w == new_console.command_prefix;
        //commandmillis = totalmillis;
        //new_console.set_buffer(w);
        //copystring(commandbuf, w, BIGSTRLEN);
        //DELETEA(commandaction);
        //DELETEA(commandicon);
        //new_console.cursor_pos = new_console.get_buffer().length();
        //commandpos = strlen(commandbuf);
        int action = 1;
        if (!consolecmd)
        {
            action = 0;//action = "say (getconsolebuffer)";
        }
        printf("Set input to %s\n", w);
        new_console.set_input(w, action);
        //commandcolour = 0;
        //commandflags = CF_EXECUTE|CF_MESSAGE;
       new_console.open_console();
        
        printf("Run\n");
        UI::editoredit(UI::geteditor("console_input", EDITORFOREVER, init, "console_window"), init);
    }
    return true;
}

#endif
