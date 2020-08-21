#include "input.h"
#include "engine.h"

InputSystem input_system = InputSystem();

ICOMMAND(0, keymap, "is", (int* code, char* key), input_system.map_key(code, key));

ICOMMAND(0, bind,     "ss", (char* key, char* action), input_system.bind_key(key, action, KeyMap::ACTION_DEFAULT, "bind"));
ICOMMAND(0, specbind, "ss", (char* key, char* action), input_system.bind_key(key, action, KeyMap::ACTION_SPECTATOR, "specbind"));
ICOMMAND(0, editbind, "ss", (char* key, char* action), input_system.bind_key(key, action, KeyMap::ACTION_EDITING, "editbind"));
ICOMMAND(0, waitbind, "ss", (char* key, char* action), input_system.bind_key(key, action, KeyMap::ACTION_WAITING, "waitbind"));
ICOMMAND(0, getbind,     "s", (char* key), input_system.get_bind(key, KeyMap::ACTION_DEFAULT));
ICOMMAND(0, getspecbind, "s", (char* key), input_system.get_bind(key, KeyMap::ACTION_SPECTATOR));
ICOMMAND(0, geteditbind, "s", (char* key), input_system.get_bind(key, KeyMap::ACTION_EDITING));
ICOMMAND(0, getwaitbind, "s", (char* key), input_system.get_bind(key, KeyMap::ACTION_WAITING));
ICOMMAND(0, searchbinds,      "sissssb", (char* action, int* limit, char* s1, char* s2, char* sep1, char* sep2, int* force), { std::string list; input_system.search_bind_list(action, KeyMap::ACTION_DEFAULT,   std::max(*limit, 0), s1, s2, sep1, sep2, list, *force != 0); result(list.c_str()); });
ICOMMAND(0, searchspecbinds,  "sissssb", (char* action, int* limit, char* s1, char* s2, char* sep1, char* sep2, int* force), { std::string list; input_system.search_bind_list(action, KeyMap::ACTION_SPECTATOR, std::max(*limit, 0), s1, s2, sep1, sep2, list, *force != 0); result(list.c_str()); });
ICOMMAND(0, searcheditbinds,  "sissssb", (char* action, int* limit, char* s1, char* s2, char* sep1, char* sep2, int* force), { std::string list; input_system.search_bind_list(action, KeyMap::ACTION_EDITING,   std::max(*limit, 0), s1, s2, sep1, sep2, list, *force != 0); result(list.c_str()); });
ICOMMAND(0, searchwaitbinds,  "sissssb", (char* action, int* limit, char* s1, char* s2, char* sep1, char* sep2, int* force), { std::string list; input_system.search_bind_list(action, KeyMap::ACTION_WAITING,   std::max(*limit, 0), s1, s2, sep1, sep2, list, *force != 0); result(list.c_str()); });

ICOMMAND(0, keyspressed, "issss", (int* limit, char* s1, char* s2, char* sep1, char* sep2), { std::string list; input_system.get_key_pressed(std::max(*limit, 0), s1, s2, sep1, sep2, list); result(list.c_str()); });

KeyMap::KeyMap()
{
    code = -1;
    name = "";
    pressed = false;
    for (int i = 0; i < NUMACTIONS; i++)
    {
        actions[i] = "";
        persist[i] = false;
    }
}

KeyMap::~KeyMap()
{
    for (int i = 0; i < NUMACTIONS; i++)
    {
        persist[i] = false;
    }
}

InputSystem::InputSystem()
{
    capslock_on = capslocked();
    numlock_on = numlocked();
}

void InputSystem::clear_keymaps()
{
    key_maps.clear();
}

const char* InputSystem::get_key_name(int code)
{
    if (key_maps.find(code) != key_maps.end())
    {
        return key_maps[code]->name.c_str();
    }
    return nullptr;
}

void InputSystem::search_bind_list(const char* action, int type, int limit, const char* c_s1, const char* c_s2, const char* c_sep1, const char* c_sep2, std::string& names, bool force)
{
    std::string name1;
    std::string name2;
    std::string lastname;
    const std::string s1 = c_s1;
    const std::string s2 = c_s2;
    const std::string sep1 = c_sep1;
    const std::string sep2 = c_sep2;

    int found = 0;

    for (const auto& key_map_pair : key_maps)
    {
        const KeyMap keymap = *key_map_pair.second;

        std::string act = type && force && keymap.actions[type].empty() ? keymap.actions[KeyMap::ACTION_DEFAULT] : keymap.actions[type];
        if (!act.empty() && !strcmp(act.c_str(), action))
        {
            if (name1.empty())
            {
                name1 = keymap.name;
            }
            else if (name2.empty())
            {
                name2 = keymap.name;
            }
            else
            {
                if (!lastname.empty())
                {
                    names += sep1 + s1 + lastname + s2;
                }
                else
                {
                    names += s1 + name1 + s2 + sep1 + s1 + name2 + s2;
                }
                lastname = keymap.name;
            }
            ++found;
            if (limit > 0 && found >= limit)
            {
                break;
            }
        }
    }
    if (!lastname.empty())
    {
        std::string sep;
        if (!sep2.empty())
        {
            sep = sep2;
        }
        else if (!sep1.empty())
        {
            sep = sep1;
        }
        names += sep + s1 + lastname + s2;
    }
    else
    {
        if (!name1.empty())
        {
            names += s1 + name1 + s2;
        }
        
        if (!name2.empty())
        {
            std::string sep;
            if (!sep2.empty())
            {
                sep = sep2;
            }
            else if (!sep1.empty())
            {
                sep = sep1;
            }
            names += sep + s1 + name2 + s2;
        }
    }
}

void InputSystem::map_key(int* code, char* key)
{
    if (identflags & IDF_WORLD)
    {
        conoutf("\frCannot Override Keymap");
        return;
    }
    if (key_maps.find(*code) != key_maps.end())
    {
        key_maps[*code]->code = *code;
        key_maps[*code]->name = key;
    }
    else
    {
        KeyMap* keymap = new KeyMap();
        keymap->code = *code;
        keymap->name = key;
        key_maps[*code] = keymap;
    }
}

const char* InputSystem::search_bind(const char* action, int type)
{
    for (const auto& key_map_pair : key_maps)
    {
        const KeyMap keymap = *key_map_pair.second;
        std::string act  = ((!keymap.actions[type].empty()) && type ? keymap.actions[KeyMap::ACTION_DEFAULT] : keymap.actions[type]);
        if (!strcmp(act.c_str(), action))
        {
            return keymap.name.c_str();
        }
    }
    return nullptr;
}

void InputSystem::get_key_pressed(int limit, const char* c_s1, const char* c_s2, const char* c_sep1, const char* c_sep2, std::string& names)
{
    std::string name1;
    std::string name2;
    std::string lastname;
    const std::string s1 = c_s1;
    const std::string s2 = c_s2;
    const std::string sep1 = c_sep1;
    const std::string sep2 = c_sep2;
    int found = 0;
    for (const auto& key_map_pair : key_maps)
    {
        const KeyMap keymap = *key_map_pair.second;

        if (keymap.pressed)
        {
            if (name1.empty())
            {
                name1 = keymap.name;
            }
            else if (name2.empty())
            {
                name2 = keymap.name;
            }
            else
            {
                if (!lastname.empty())
                {
                    names += sep1 + s1 + lastname + s2;
                }
                else
                {
                    names += s1 + name1 + s2 + sep1 + s1 + name2 + s2;
                }
                lastname = keymap.name;
            }
            ++found;
    
            if (limit > 0 && found >= limit)
            {
                break;
            }
        }
    }

    if (!lastname.empty())
    {
        std::string sep;
        if (!sep2.empty())
        {
            sep = sep2;
        }
        else if (!sep1.empty())
        {
            sep = sep1;
        }
        
        names += sep + s1 + lastname + s2;
    }
    else
    {
        if (!name1.empty())
        {
            names += s1 + name1 + s2;
        }
        
        if (!name2.empty())
        {
            std::string sep;
            if (!sep2.empty())
            {
                sep = sep2;
            }
            else if (!sep1.empty())
            {
                sep = sep1;
            }
            
            names += sep + s1 + name2 + s2;
        }
    }
}

int InputSystem::find_key_code(char* key)
{
    for (const auto& key_map_pair : key_maps)
    {
        const KeyMap keymap = *key_map_pair.second;
        if (!strcasecmp(keymap.name.c_str(), key))
        {
            return key_map_pair.first;
        }
    }
    return 0;
}

KeyMap* InputSystem::find_bind(char* key)
{
    for (auto& key_map_pair : key_maps)
    {
        KeyMap* keymap = key_map_pair.second;
        if (!strcasecmp(keymap->name.c_str(), key))
        {
            return keymap;
        }
    }
    return nullptr;
}

// Returns nullptr if no keymap with <code> exists
KeyMap* InputSystem::get_key(int code)
{
    if (key_maps.find(code) != key_maps.end())
    {
        return key_maps[code];
    }
    return nullptr;
}

void InputSystem::get_bind(char* key, int type)
{
    KeyMap* keymap = find_bind(key);
    std::string result_str;
    if (keymap != nullptr)
    {
        result_str = !keymap->actions[type].empty() && type ? keymap->actions[KeyMap::ACTION_DEFAULT] : keymap->actions[type];
    }
    result(result_str.c_str());
}

void InputSystem::bind_key(char* key, char* action, int state, const char* cmd)
{
    if (identflags & IDF_WORLD)
    {
        conoutf("\frCannot Override %s \"%s\"", cmd, key);
        return;
    }
    KeyMap* keymap = find_bind(key);
    if (keymap == nullptr)
    {
        conoutf("\frUnknown Key \"%s\"", key);
        return;
    }

    std::string& binding = keymap->actions[state];
    bool* persist = &keymap->persist[state];

    if (!key_pressed || key_action != binding)
    {
        binding.clear();
    }

    // trim white-space to make searchbinds more reliable
    while (iscubespace(*action))
    {
        action++;
    }

    int len = strlen(action);
    while (len > 0 && iscubespace(action[len - 1]))
    {
        len--;
    }

    binding = action;
    *persist = initing != INIT_DEFAULTS;
    changed_keys = totalmillis;
}

const char* InputSystem::add_release_action(char* s)
{
    if (!key_pressed)
    {
        delete[] s;
        return nullptr;
    }

    key_release_actions.push_back(KeyReleaseAction());
    KeyReleaseAction& release_action = key_release_actions.back();
    release_action.key = key_pressed;
    release_action.action = s;
    return key_pressed->name.c_str();
}

void InputSystem::on_release(const char* s)
{
    add_release_action(newstring(s));
}
ICOMMAND(0, onrelease, "s", (const char* s), input_system.on_release(s));

void InputSystem::exec_bind(KeyMap& key, bool isdown)
{
    for (int i = 0; i < int(key_release_actions.size()); i++)
    {
        KeyReleaseAction& release_action = key_release_actions[i];   
        if (release_action.key == &key)
        {
            if (!isdown)
            {
                execute(release_action.action);
            }
            delete[] release_action.action;
            key_release_actions.erase(key_release_actions.begin() + i);
        }
    }

    if (isdown)
    {
        int state = KeyMap::ACTION_DEFAULT;
        switch (client::state())
        {
            case CS_ALIVE:
            case CS_DEAD:
            default:
                break;
            case CS_SPECTATOR:
                state = KeyMap::ACTION_SPECTATOR;
                break;
            case CS_EDITING:
                state = KeyMap::ACTION_EDITING;
                break;
            case CS_WAITING:
                state = KeyMap::ACTION_WAITING;
                break;
        }
        
        std::string& action = key.actions[state][0] != 0 ? key.actions[state] : key.actions[KeyMap::ACTION_DEFAULT];
        key_action = action;
        key_pressed = &key;
        execute(key_action.c_str());
        key_pressed = nullptr;
        if (key_action != action)
        {
            key_action.clear();
        }
    }
    key.pressed = isdown;
}

std::string InputSystem::get_save_binds_string()
{
    std::string save_binds_string;
    static const std::string cmds[4] = { "bind", "specbind", "editbind", "waitbind" };
    static const char SPACE = ' ';
    static const char OPEN_BRACKET = '[';
    static const char CLOSE_BRACKET = ']';
    static const char LINE_BREAK = '\n';

    //TODO: sort the binds
    for (int i = 0; i < 4; i++)
    {
        bool found = false;
        for (const auto& key_map_pair : key_maps)
        {
            const KeyMap* keymap = key_map_pair.second;
            if (keymap->persist[i])
            {
                if (keymap->actions[i].empty())
                {
                    save_binds_string += cmds[i] + SPACE + std::string(escapestring(keymap->name.c_str())) + SPACE + OPEN_BRACKET + CLOSE_BRACKET + LINE_BREAK;
                }
                else if (validateblock(keymap->actions[i].c_str()))
                {
                    save_binds_string += cmds[i] + SPACE + std::string(escapestring(keymap->name.c_str())) + SPACE + OPEN_BRACKET + keymap->actions[i] + CLOSE_BRACKET + LINE_BREAK;
                }
                else
                {
                    save_binds_string += cmds[i] + SPACE + std::string(escapestring(keymap->name.c_str())) + SPACE + std::string(escapestring(keymap->actions[i].c_str())) + LINE_BREAK;
                }
                found = true;
            }
        }

        if (found)
        {
            save_binds_string += LINE_BREAK;
        }
    }
    return save_binds_string;
}

#define keyintercept(name,body) \
{ \
    static bool key##name = false; \
    if (SDL_GetModState() & MOD_ALTS && isdown) \
    { \
        if (!key##name) \
        { \
            body; \
            key##name = true; \
        } \
        return; \
    } \
    if (!isdown && key##name) \
    { \
        key##name = false; \
        return; \
    } \
}

void InputSystem::process_key(int code, bool isdown)
{
    switch (code)
    {
#ifndef __APPLE__
        case SDLK_q:
#else
        case SDLK_F4:
#endif
            keyintercept(quit, quit());
            break;
        case SDLK_RETURN:
            keyintercept(fullscreen, setfullscreen(!(SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN)));
            break;
#if defined(WIN32) || defined(__APPLE__)
        case SDLK_TAB:
            keyintercept(iconify, SDL_MinimizeWindow(screen));
            break;
#endif
        case SDLK_CAPSLOCK:
            if (!isdown)
            {
                capslock_on = capslocked();
            }
            break;
        case SDLK_NUMLOCKCLEAR:
            if (!isdown)
            {
                numlock_on = numlocked();
            }
            break;
        default:
            break;
    }

    KeyMap* key = get_key(code);
    //TODO: move console input into its own status, thus removing the entaglement at least partially
    if ((key != nullptr && key->pressed)
        || (!new_console.process_key(code, isdown) && !hud::keypress(code, isdown) && key != nullptr))
    {
        input_system.exec_bind(*key, isdown);
    }
}

void InputSystem::process_text_input(const char* str, int len)
{
    if (!hud::textinput(str, len))
    {
        new_console.process_text_input(str, len);
    }
}

#ifdef __APPLE__
extern bool mac_capslock();
extern bool mac_numlock();
#endif

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/XKBlib.h>
#endif

bool InputSystem::capslocked()
{
#ifdef WIN32
    if (GetKeyState(VK_CAPITAL))
    {
        return true;
    }
#elif defined(__APPLE__)
    if (mac_capslock())
    {
        return true;
    }
#else
    Display* d = XOpenDisplay((char*)0);
    if (d != nullptr)
    {
        uint n = 0;
        XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
        XCloseDisplay(d);
        return (n&0x01) != 0;
    }
#endif
    return false;
}
ICOMMAND(0, getcapslock, "", (), intret(input_system.capslock_on ? 1 : 0));

bool InputSystem::numlocked()
{
 #ifdef WIN32
    if (GetKeyState(VK_NUMLOCK))
    {
        return true;
    }
#elif defined(__APPLE__)
    if (mac_numlock())
    {
        return true;
    }
#else
    Display* d = XOpenDisplay((char*)0);
    if (d != nullptr)
    {
        uint n = 0;
        XkbGetIndicatorState(d, XkbUseCoreKbd, &n);
        XCloseDisplay(d);
        return (n&0x02) != 0;
    }
#endif
    return false;
}
ICOMMAND(0, getnumlock, "", (), intret(input_system.numlock_on ? 1 : 0));


