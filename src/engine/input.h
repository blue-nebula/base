#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <map>
#include <vector>

class KeyMap
{
public:
    enum
    {
        ACTION_DEFAULT = 0,
        ACTION_SPECTATOR = 1,
        ACTION_EDITING = 2,
        ACTION_WAITING = 3,
        NUMACTIONS = 4
    };

    int code;
    std::string name;
    std::string actions[NUMACTIONS];
    bool pressed;
    bool persist[NUMACTIONS];

    KeyMap();
    ~KeyMap();
};

class KeyReleaseAction
{
public:
    KeyMap* key;
    char* action;
};

class InputSystem
{
private:
    KeyMap* key_pressed = nullptr;
    std::string key_action;
    std::map<int, KeyMap*> key_maps;
    std::vector<KeyReleaseAction> key_release_actions;
    bool capslocked();
    bool numlocked();
public:
    int changed_keys = 0;
    bool capslock_on = false;
    bool numlock_on = false;

    InputSystem();

    void clear_keymaps();        
    const char* get_key_name(int code);
    void search_bind_list(const char* action, int type, int limit, const char* c_s1, const char* c_s2, const char* c_sep1, const char* c_sep2, std::string& names, bool force = true);
    const char* search_bind(const char* action, int type);
    void get_key_pressed(int limit, const char* c_s1, const char* c_s2, const char* c_sep1, const char* c_sep2, std::string& names);
    int find_key_code(char* key);
    KeyMap* find_bind(char* key);
    KeyMap* get_key(int code);
    
    void get_bind(char* key, int type);
    void bind_key(char* key, char* action, int state, const char* cmd);
    void map_key(int* code, char* key);

    const char* add_release_action(char* s);
    void on_release(const char* s);
    void exec_bind(KeyMap& key, bool isdown);

    std::string get_save_binds_string();

    void process_key(int code, bool isdown);
    void process_text_input(const char* str, int len);
};

#endif // INPUT_H
