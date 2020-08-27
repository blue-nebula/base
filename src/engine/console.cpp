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

/*
bool CompletionBase::can_complete(Console& console)
{
    printf("Lmao what the fuck are you doing?\n");
    return false;
}
*/

bool InputHistory::move(const int lines)
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

bool History::move(const int lines)
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
        const int max_pos = num_linebreaks - (*idents["consize"].storage.i + *idents["conoverflow"].storage.i);
        scroll_pos = std::min(scroll_pos + lines, max_pos);
    }
    else
    {
        // lines is smaller than 0, go down
        scroll_pos = std::max(scroll_pos + lines, 0);
    }

    // make sure scrollpos never gets below 0
    scroll_pos = std::max(scroll_pos, 0);

    bool moved = scroll_pos != prev_scroll_pos;
    if (moved)
    {
        recalc_scroll_info();
    }

    return moved;
}

void History::remove(const int idx)
{
    num_linebreaks -= h[idx].num_linebreaks;
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

void History::save(ConsoleLine& line)
{
    calculate_wordwrap(line);
    num_linebreaks += line.num_linebreaks;

    if (int(h.size()) > 1000)
    {
        num_linebreaks -= h.back().num_linebreaks;
        h.pop_back();
    }
    h.push_front(line);

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
        std::vector<std::pair<int, std::string>> wraps = get_text_wraps(line.text.c_str(), max_line_width);

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
    line.num_linebreaks = line.lines.size();
}

void History::calculate_all_wordwraps()
{
    //TODO: measure time
    num_linebreaks = 0;
    for (auto& line : h)
    {
        calculate_wordwrap(line);
        num_linebreaks += line.num_linebreaks;
    }
}

Console::Console()
{
    chat_history.type_background_colors[CON_CHAT_WHISPER] = {.7f, .7f, .7f, .2f};
    chat_history.type_background_colors[CON_CHAT_TEAM] = {0, 0, .1f, .8f};

    register_completion(new PlayerNameCompletion());
    register_completion(new CommandCompletion());
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
    switch (selected_hist)
    {
        case HIST_CHAT:
            return chat_history;
        case HIST_CONSOLE:
            return console_history;
        default:
            return all_history;
    }
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
        int i = 0;
        for (auto* completion_engine : completions_engines)
        {
            if (completion_engine->can_complete(*this))
            {
                printf("Can complete with %d\n", i);
                curr_completions = completion_engine->get_completions(this->buffer);
                if (int(curr_completions.size()) > 0)
                {
                    break;
                }
            }
            i++;
        }
    }
}

void Console::set_max_line_width(int w)
{
    // it's assumed that line_width of all histories are in sync
    int prev_line_width = chat_history.max_line_width;
    chat_history.max_line_width = w;
    console_history.max_line_width = w;

    if (prev_line_width != w)
    {
        chat_history.calculate_all_wordwraps();
        console_history.calculate_all_wordwraps();
    }
}

std::string Console::get_buffer()
{
    return buffer;
}
ICOMMAND(0, getconsolebuffer, "", (), stringret(newstring(new_console.get_buffer().c_str())));

void Console::print(int type, const std::string text, const std::string raw_text)
{
    //psrintf("Adding %s\n", text.c_str());
    ///std::cout << "Adding " << text << " at " << chat_history.h.size() << std::endl;

    if (text.empty())
    {
        printf("Error printing empty text of type %d\n", type);
        return;
    }
    
    //get_wraps_textf(text.c_str(), chat_history.line_width, 1);

    ConsoleLine line = ConsoleLine();
    line.text = text;
    line.raw_text = raw_text;
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
            chat_history.save(line);
            break;
        case CON_DEBUG_ERROR: /* FALLTHROUGH */
            unseen_error_messages++;
        default:
            console_history.save(line);
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
            preview_history.save(line);
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
            execute(curr_action.c_str());
        }

        // reset scrolling
        curr_hist().scroll_pos = 0;
        curr_hist().recalc_scroll_info();
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
    curr_action = "";
    curr_icon = "";
    input_history.hist_pos = -1;
}

void Console::insert_in_buffer(const std::string text)
{
    int maxlen = std::min(client::maxmsglen(), max_buffer_len);

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
        cursor_pos = int(buffer.length());
    }
    else
    {
        std::string new_buffer = get_buffer();
        new_buffer.insert(std::max(0, cursor_pos), text);
        set_buffer(new_buffer);

        cursor_pos += insert_len;
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
        else if (buffer[0] == search_prefix)
        {
            return MODE_SEARCH;
        }
    }
    
    return MODE_NONE;
}

std::string Console::get_info_bar_text()
{
    std::string text = "";
    switch (get_mode())
    {
        case MODE_SEARCH:
            text = "\fgSearch Mode: \fwFound ";
            text += std::to_string(search_results.size()) + " match" + (search_results.size() == 1 ? "." : "es.");
            break;
        case MODE_COMMAND:
            text = "\fbCommand Mode";
            break;
    }
    return text;
}

void Console::update_search()
{
    if (get_buffer().empty())
    {
        return;
    }

    const bool smartcase = true;

    std::string search_term = get_buffer().substr(1, -1);
    bool case_sensitive_search = true;
    // if smartcase is on, and the search contains an uppercase letter,
    // search is case sensitive
    if (smartcase && !std::all_of(search_term.begin(), search_term.end(), islower))
    {
        case_sensitive_search = true;
    }
    else
    {
        case_sensitive_search = false;
        //TODO: make search_term lowercase
        std::transform(search_term.begin(), search_term.end(), search_term.begin(),
            [](unsigned char c){ return std::tolower(c); });
    }

    search_results.clear();
    for (const auto& line : curr_hist().h)
    {
        std::string line_text = line.type != CON_CHAT ? line.text : line.raw_text;
        if (!case_sensitive_search)
        {
            std::transform(line_text.begin(), line_text.end(), line_text.begin(),
                [](unsigned char c){ return std::tolower(c); });
        }

        if (line_text.find(search_term) != std::string::npos)
        {
            search_results.push_back(line.text);
        }
    }

    //printf("%d: %s: %d\n", case_sensitive_search, search_term.c_str(), int(search_results.size()));
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

void inputcommand(char *init, char *action = NULL, char *icon = NULL, int colour = 0, char *flags = NULL) // turns input to the command line on or off
{
    new_console.set_input(init != nullptr ? init : "", 
                        action != nullptr ? action : "", 
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

void Console::set_input(std::string init, std::string action, std::string icon)
{
    open_console();

    set_buffer(init);

    new_console.cursor_pos = -1;
    curr_action = action;
    curr_icon = icon;
}

ICOMMAND(0, saycommand, "C", (char *init), inputcommand(init));
ICOMMAND(0, inputcommand, "sssis", (char *init, char *action, char *icon, int *colour, char *flags), inputcommand(init, action, icon, *colour, flags));

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
    size_t decoded = decodeutf8((uchar *)&buf[start], len-1-start, (const uchar *)cb, cblen);
   

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
                if (get_buffer().length() != 0)
                {
                    cursor_pos = 0;
                }
                break;

            case SDLK_END:
                cursor_pos = -1;
                break;

            case SDLK_DELETE:
            {
                int len = int(get_buffer().length());
                
                if (cursor_pos < 0)
                {
                    break;
                }

                std::string new_buffer = get_buffer();
                new_buffer.erase(new_buffer.begin() + cursor_pos);
                set_buffer(new_buffer);

                resetcomplete();
                if (cursor_pos >= len - 1)
                {
                    cursor_pos = -1;
                }

                break;
            }

            case SDLK_BACKSPACE:
            {
                int len = int(get_buffer().length());
                int i = cursor_pos >= 0 ? cursor_pos : len;

                if (i < 1)
                {
                    break;
                }

                std::string new_buffer = get_buffer();
                new_buffer.erase(new_buffer.begin() + (i - 1));
                set_buffer(new_buffer);
                
                resetcomplete();
                if (cursor_pos > 0)
                {
                    cursor_pos--;
                }
                else if (cursor_pos == 0 && len <= 1)
                {
                    cursor_pos = -1;
                }
                break;
            }

            case -4:
                if (SDL_GetModState() & KMOD_SHIFT)
                {
                    curr_hist().move(10);
                    break;
                }
                curr_hist().move(1);
                break;
            case -5:
                if (SDL_GetModState() & KMOD_SHIFT)
                {
                    curr_hist().move(-10);
                }
                curr_hist().move(-1);
                break;
            case SDLK_PAGEUP:
                curr_hist().move(10);
                break;
            case SDLK_PAGEDOWN:
                curr_hist().move(-10);
                break;

            case SDLK_LEFT:
                if (cursor_pos > 0)
                {
                    cursor_pos--;
                }
                else if (cursor_pos < 0)
                {
                    cursor_pos = int(get_buffer().length()) - 1;
                }
                break;

            case SDLK_RIGHT:
                if (cursor_pos >= 0 && ++cursor_pos >= int(get_buffer().length()))
                {
                    cursor_pos = -1;
                }
                break;

            case SDLK_UP:
                if (SDL_GetModState() & KMOD_CTRL)
                {
                    int speed = 1;
                    // accelerate scroll speed with shift
                    if (SDL_GetModState() & KMOD_SHIFT)
                    {
                        speed *= 10;
                    }

                    curr_hist().move(speed);
                    break;
                }
                
                if (input_history.move(1))
                {
                    set_buffer(input_history.current_line.text);
                    curr_icon = input_history.current_line.icon;
                    curr_action = input_history.current_line.action;
                }
                break;

            case SDLK_DOWN:
                if (SDL_GetModState() & KMOD_CTRL)
                {
                    int speed = -1;
                    // accelerate scroll speed with shift
                    if (SDL_GetModState() & KMOD_SHIFT)
                    {
                        speed *= 10;
                    }

                    curr_hist().move(speed);
                    break;
                }

                if (input_history.move(-1))
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
            
            close_console();
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
    return curr_icon;
}

void Console::clear_curr_hist()
{
    curr_hist().clear();
}
ICOMMAND(0, clear, "", (), new_console.clear_curr_hist());

void Console::register_completion(CompletionBase* completion)
{
    completions_engines.push_back(completion);
}

std::vector<CompletionEntryBase*> Console::get_curr_completions()
{
    return curr_completions;
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
        std::string action = "";
        if (!consolecmd)
        {
            action = "say (getconsolebuffer)";
        }
        printf("Set input to %s\n", w);
        new_console.set_input(w, action);
        //commandcolour = 0;
        //commandflags = CF_EXECUTE|CF_MESSAGE;
        /*
        hline *h = NULL;
        if (!new_console.buffer.empty())
        {
            if(history.empty() || history.last()->shouldsave())
            {
                if(maxhistory && history.length() >= maxhistory)
                {
                    loopi(history.length()-maxhistory+1) delete history[i];
                    history.remove(0, history.length()-maxhistory+1);
                }
                history.add(h = new hline)->save();
            }
            else h = history.last();
        }
        histpos = history.length();
        */
        new_console.open_console();

        /*if(h)
        {
            interactive = true;
            h->run();
            interactive = false;
        }*/
        printf("Run\n");
        UI::editoredit(UI::geteditor("console_input", EDITORFOREVER, init, "console_window"), init);
    }
    return true;
}
#endif
