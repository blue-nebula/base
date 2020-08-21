// console.cpp: the console buffer, its display, and command line control
#include <string>
#include <algorithm>

#include "console.h"
#include "engine.h"
#include "game.h"

bool InputHistory::go_up()
{
    int prev_history_pos = current_history_pos;
    current_history_pos = std::min(current_history_pos + 1, int(history.size()));

    current_line = history[current_history_pos];
    return current_history_pos != prev_history_pos;
}

bool InputHistory::go_down()
{
    int prev_history_pos = current_history_pos;
    current_history_pos = std::max(current_history_pos - 1, 0);

    current_line = history[current_history_pos];
    return current_history_pos != prev_history_pos;
}

void InputHistory::save(InputHistoryLine line)
{
    history.push_front(line);
}

bool History::move(int lines)
{
    if (lines == 0)
    {
        return false;
    }

    int prev_scroll_pos = scroll_pos;
    
    if (lines > 0)
    {
        // go up
        //TODO: Find alternative to using idents hashnameset
        int max_pos = int(h.size()) - (*idents["consize"].storage.i + *idents["conoverflow"].storage.i);
        scroll_pos = std::min(scroll_pos + lines, max_pos);
    }
    else
    {
        // lines is smaller than 0, go down
        scroll_pos = std::max(scroll_pos + lines, 0);
    }

    // make sure scrollpos never gets below 0
    scroll_pos = std::max(scroll_pos, 0);

    return scroll_pos != prev_scroll_pos;
}

void History::save(ConsoleLine line)
{
    h.push_front(line);
}

History& Console::curr_hist()
{
    switch (selected_hist)
    {
        case HIST_CHAT:
            return chat_history;
        case HIST_GAME:
            return game_history;
        case HIST_DEBUG:
            return debug_history;
        default:
            return all_history;
    }
}

void Console::set_buffer(std::string text)
{
    buffer = text;
    // update search or completion
    
    switch (get_mode())
    {
        case MODE_COMMAND:
            //update_completion();
            break;
        case MODE_SEARCH:
            update_search();
            break;
    }
}
std::string Console::get_buffer()
{
    return buffer;
}

void Console::print(int type, const std::string text, const std::string raw_text)
{
    ConsoleLine line = ConsoleLine();
    line.text = text;
    line.raw_text = raw_text;
    line.type = type;
    line.reftime = totalmillis;
    line.out_time = totalmillis;
    line.real_time = clocktime;

    switch (line.type)
    {
        case CON_CHAT:
        case CON_CHAT_TEAM:
        case CON_CHAT_WHISPER:
            chat_history.save(line);
            break;
        case CON_FRAG:
            game_history.save(line);
            break;
        case CON_DEBUG:
            debug_history.save(line);
            break;
        default:
            all_history.save(line);
            break;
    }
}

void Console::run_buffer()
{
    if (buffer[0] == command_prefix)
    {
        execute(buffer.c_str() + 1);
    }
    else
    {
        client::toserver(0, buffer.c_str());
    }
    // reset scrolling
    curr_hist().scroll_pos = 0;
}

void Console::open_console()
{
}

void Console::close_console()
{
    input_history.current_history_pos = -1;
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

    printf("%d: %s: %d\n", case_sensitive_search, search_term.c_str(), int(search_results.size()));
}

Console new_console = Console();

reversequeue<cline, MAXCONLINES> conlines;

int commandmillis = -1;
//bigstring commandbuf;
char *commandaction = NULL, *commandicon = NULL;
enum { CF_COMPLETE = 1<<0, CF_EXECUTE = 1<<1, CF_MESSAGE = 1<<2 };
int commandflags = 0;//commandpos = -1, commandcolour = 0;
int commandcolour = 0;

void complete(char* s, size_t s_size, const char* cmdprefix);

void conline(int type, const char *sf, int n)
{
    new_console.print(type, sf);
}

void inputcommand(char *init, char *action = NULL, char *icon = NULL, int colour = 0, char *flags = NULL) // turns input to the command line on or off
{
    commandmillis = init ? totalmillis : -totalmillis;
    textinput(commandmillis >= 0, TI_CONSOLE);
    keyrepeat(commandmillis >= 0, KR_CONSOLE);
    if (init != nullptr)
    {
        new_console.set_buffer(init);
    }
    DELETEA(commandaction);
    DELETEA(commandicon);
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

    char* clipboard_text = SDL_GetClipboardText();
    if (clipboard_text == nullptr)
    {
        return false;
    }

    size_t clipboard_len = strlen(clipboard_text);
    printf("%d: %s\n", int(clipboard_len), clipboard_text);
    
    new_console.insert_in_buffer(clipboard_text);

    // documentation says this has to be done
    SDL_free(clipboard_text);

    return true;
}

SVAR(0, commandbuffer, "");

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
    if (commandmillis < 0)
    {
        return false;
    }

    resetcomplete();
    new_console.insert_in_buffer(str);

    return true;
}

bool Console::process_key(int code, bool isdown)
{
    if (commandmillis < 0)
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
                if (new_console.get_buffer().length() != 0)
                {
                    new_console.cursor_pos = 0;
                }
                break;

            case SDLK_END:
                new_console.cursor_pos = -1;
                break;

            case SDLK_DELETE:
            {
                int len = int(new_console.get_buffer().length());
                
                if (new_console.cursor_pos < 0)
                {
                    break;
                }

                std::string new_buffer = new_console.get_buffer();
                new_buffer.erase(new_buffer.begin() + new_console.cursor_pos);
                new_console.set_buffer(new_buffer);

                resetcomplete();
                if (new_console.cursor_pos >= len - 1)
                {
                    new_console.cursor_pos = -1;
                }

                break;
            }

            case SDLK_BACKSPACE:
            {
                int len = int(new_console.get_buffer().length());
                int i = new_console.cursor_pos >= 0 ? new_console.cursor_pos : len;

                if (i < 1)
                {
                    break;
                }

                std::string new_buffer = new_console.get_buffer();
                new_buffer.erase(new_buffer.begin() + (i - 1));
                new_console.set_buffer(new_buffer);
                
                resetcomplete();
                if (new_console.cursor_pos > 0)
                {
                    new_console.cursor_pos--;
                }
                else if (new_console.cursor_pos == 0 && len <= 1)
                {
                    new_console.cursor_pos = -1;
                }
                break;
            }

            case -4:
                new_console.curr_hist().move(1);
                break;
            case -5:
                new_console.curr_hist().move(-1);
                break;
            case SDLK_PAGEUP:
                new_console.curr_hist().move(10);
                break;
            case SDLK_PAGEDOWN:
                new_console.curr_hist().move(-10);
                break;

            case SDLK_LEFT:
                if (new_console.cursor_pos > 0)
                {
                    new_console.cursor_pos--;
                }
                else if (new_console.cursor_pos < 0)
                {
                    new_console.cursor_pos = int(new_console.get_buffer().length()) - 1;
                }
                break;

            case SDLK_RIGHT:
                if (new_console.cursor_pos >= 0 && ++new_console.cursor_pos >= int(new_console.get_buffer().length()))
                {
                    new_console.cursor_pos = -1;
                }
                break;

            case SDLK_UP:
                if (new_console.input_history.go_up())
                {
                    new_console.set_buffer(new_console.input_history.current_line.text);
                }
                break;

            case SDLK_DOWN:
                if (new_console.input_history.go_down())
                {
                    new_console.set_buffer(new_console.input_history.current_line.text);
                }
                break;

            case SDLK_TAB:
                //TODO: Implement completion
                /*if(commandflags&CF_COMPLETE)
                {
                    complete(commandbuf, BIGSTRLEN, commandflags&CF_EXECUTE ? "/" : NULL);
                    if(commandpos>=0 && commandpos>=(int)strlen(commandbuf)) commandpos = -1;
                }*/
                printf("Completion isn't implemented yet\n");
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
                selected_hist = HIST_GAME;
                break;
            case SDLK_F3:
                selected_hist = HIST_DEBUG;
                break;
        }
    }
    else
    {
        if (code == SDLK_RETURN || code == SDLK_KP_ENTER)
        {
            if (!new_console.get_buffer().empty())
            {
                // save this line to the history
                InputHistoryLine line = InputHistoryLine();
                line.text = new_console.get_buffer();
                new_console.input_history.save(line);
            }
            inputcommand(nullptr);
                
            interactive = true;
            new_console.run_buffer();
            interactive = false;
            
            new_console.close_console();
        }
        else if (code == SDLK_ESCAPE || (code < 0 && code != -5 && code != -4))
        {
            inputcommand(nullptr);
            new_console.close_console();
        }
    }

    return true;
}

char *getcurcommand()
{
    char* buf = newstring(new_console.get_buffer().c_str());
    return commandmillis > 0 ? buf : (char *)nullptr;//commandbuf : (char *)NULL;
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
        if(type!=FILES_DIR || millis >= commandmillis) return;
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
        commandmillis = totalmillis;
        new_console.set_buffer(w);
        //copystring(commandbuf, w, BIGSTRLEN);
        DELETEA(commandaction);
        DELETEA(commandicon);
        new_console.cursor_pos = new_console.get_buffer().length();
        //commandpos = strlen(commandbuf);
        if(!consolecmd) commandaction = newstring("say $commandbuffer");
        commandcolour = 0;
        commandflags = CF_EXECUTE|CF_MESSAGE;
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
        inputcommand(NULL);

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
