// menus.cpp: ingame menu system (also used for scores and serverlist)
#include "engine.h"

VAR(0, guipasses, 1, -1, -1);

struct menu;

static guient *cgui = NULL;
static menu *cmenu = NULL;

struct menu : guicb
{
    char *name, *header;
    uint *contents, *init_script;
    int passes, menu_tab, menu_start;
    bool world, use_input, use_title, use_bg_fx, built_in, *keep;

    menu() : name(NULL), header(NULL), contents(NULL), init_script(NULL), passes(0), menu_tab(0), menu_start(0), world(false), use_input(true), use_title(true), use_bg_fx(true), built_in(false), keep(NULL) {}

    void gui(guient &g, bool firstpass)
    {
        cgui = &g;
        cmenu = this;
        guipasses = passes;
        if(!passes) ui_action_on = false;
        int oldflags = identflags;
        if(world && !built_in) identflags |= IDF_WORLD;
        if(init_script) execute(init_script);
        cgui->start(menu_start, &menu_tab, use_input, use_title, use_bg_fx);
        cgui->tab(header ? header : name);
        if(contents) execute(contents);
        cgui->end();
        identflags = oldflags;
        guipasses = -1;
        cmenu = NULL;
        cgui = NULL;
        if((++passes) <= 0) passes = 1;
    }

    virtual void clear() {}
};

struct delayedupdate
{
    enum
    {
        INT,
        FLOAT,
        STRING,
        ACTION
    } type;
    ident *id;
    union
    {
        int i;
        float f;
        char *s;
    } val;
    bool world;
    delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
    ~delayedupdate() { if(type == STRING || type == ACTION) DELETEA(val.s); }

    void schedule(const char *s) { type = ACTION; val.s = newstring(s); world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, int i) { type = INT; id = var; val.i = i; world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; world = (identflags&IDF_WORLD)!=0; }
    void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); world = (identflags&IDF_WORLD)!=0; }

    int getint() const
    {
        switch(type)
        {
            case INT: return val.i;
            case FLOAT: return int(val.f);
            case STRING: return int(strtol(val.s, NULL, 0));
            default: return 0;
        }
    }

    float getfloat() const
    {
        switch(type)
        {
            case INT: return float(val.i);
            case FLOAT: return val.f;
            case STRING: return float(parsefloat(val.s));
            default: return 0;
        }
    }

    const char *getstring() const
    {
        switch(type)
        {
            case INT: return intstr(val.i);
            case FLOAT: return intstr(int(floor(val.f)));
            case STRING: return val.s;
            default: return "";
        }
    }

    void run()
    {
        int _oldflags = identflags;
        if(world) identflags |= IDF_WORLD;
        if(type == ACTION) { if(val.s) execute(val.s); }
        else if(id) switch(id->type)
        {
            case ID_VAR: setvarchecked(id, getint()); break;
            case ID_FVAR: setfvarchecked(id, getfloat()); break;
            case ID_SVAR: setsvarchecked(id, getstring()); break;
            case ID_ALIAS: alias(id->name, getstring()); break;
        }
        identflags = _oldflags;
    }
};

static hashnameset<menu> menus;
static vector<menu *> menustack;
static vector<delayedupdate> updatelater;
static bool shouldclearmenu = true, clearlater = false;

bool pop_ui(bool skip = true)
{
    menu *m = menustack.last();
    if(skip && m->keep && *m->keep) return false;
    menustack.pop();
    m->passes = 0;
    if(m->keep) *m->keep = false;
    m->clear();
    return true;
}

bool remove_ui(menu *m)
{
    loopv(menustack) if(menustack[i] == m)
    {
        menustack.remove(i);
        m->passes = 0;
        if(m->keep) *m->keep = false;
        m->clear();
        return true;
    }
    return false;
}

bool push_ui(menu *m, int pos = -1, int tab = 0, bool *keep = NULL)
{
    if(menustack.empty()) resetcursor();
    if(pos < 0) menustack.add(m);
    else menustack.insert(pos, m);
    if(m)
    {
        m->passes = 0;
        m->menu_start = totalmillis;
        if(tab > 0) m->menu_tab = tab;
        m->use_title = tab >= 0 ? true : false;
        m->world = (identflags&IDF_WORLD)!=0;
        m->keep = keep;
        return true;
    }
    return false;
}

bool restore_ui(int pos, int tab = 0, bool *keep = NULL)
{
    int clear = menustack.length()-pos-1;
    loopi(clear) pop_ui();
    menu *m = menustack.last();
    if(m)
    {
        if(clear)
        {
            m->passes = 0;
            m->menu_start = totalmillis;
        }
        if(tab > 0) m->menu_tab = tab;
        m->world = (identflags&IDF_WORLD)!=0;
        m->keep = keep;
        return clear != 0;
    }
    return false;
}

bool showgui(const char *name, int tab, bool *keep)
{
    menu *m = menus.access(name);
    if(!m) return false;
    int pos = menustack.find(m);
    if(pos < 0) push_ui(m, -1, tab, keep);
    else if(!restore_ui(pos, tab, keep)) return true;
    playsound(S_GUIPRESS, camera1->o, camera1, SND_FORCED);
    return true;
}

extern bool closetexgui();
int cleargui(int n, bool skip)
{
    if(closetexgui()) n--;
    int clear = menustack.length();
    if(n>0) clear = min(clear, n);
    loopi(clear) if(!pop_ui(skip)) break;
    if(!menustack.empty()) restore_ui(menustack.length() - 1);
    return clear;
}

void cleargui_(int *n)
{
    intret(cleargui(*n, false));
}

void ui_show_title(int *n)
{
    if(!cmenu) return;
    cmenu->use_title = *n ? true : false;
}

void ui_show_bg_fx(int *n)
{
    if(!cmenu) return;
    cmenu->use_bg_fx = *n ? true : false;
}

void ui_stay_open(uint *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

void ui_no_cursor_fx(uint *contents)
{
    if(!cgui) return;
    cgui->allowcursorfx(false);
    execute(contents);
    cgui->allowcursorfx(true);
}

void ui_no_hit_fx(uint *contents)
{
    if(!cgui) return;
    cgui->allowhitfx(false);
    execute(contents);
    cgui->allowhitfx(true);
}

void ui_no_skin_fx(uint *contents)
{
    if(!cgui) return;
    cgui->allowskinfx(false);
    execute(contents);
    cgui->allowskinfx(true);
}

SVAR(0, guirollovername, "");
SVAR(0, guirolloveraction, "");
SVAR(0, guirollovertype, "");

void ui_button(char *name, char *action, char *altact, char *icon, int *colour, int *icolour, int *wrap, int *faded, char *oicon, int *ocolour)
{
    if(!cgui) return;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, *icon ? icon : NULL, *icolour >= 0 ? *icolour : 0xFFFFFF, *wrap > 0 ? *wrap : -1, *faded != 0, *oicon ? oicon : NULL, *ocolour >= 0 ? *ocolour : 0xFFFFFF);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "ui_button", true);
    }
}

void ui_image(char *path, char *action, float *scale, int *overlaid, char *altpath, char *altact, int *colour, char *opath, int *ocolour)
{
    if(!cgui) return;
    Texture *t = path && *path ? textureload(path, 0, true, false) : NULL, *o = opath && *opath ? textureload(opath, 0, true, false) : NULL;
    if(t == notexture && *altpath) t = textureload(altpath, 0, true, false);
    if(o == notexture) o = NULL;
    int ret = cgui->image(t, *scale, *overlaid!=0, *colour >= 0 ? *colour : 0xFFFFFF, o, *ocolour >= 0 ? *ocolour : 0xFFFFFF);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", path, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "ui_image", true);
    }
}

void ui_slice(char *path, char *action, float *scale, float *start, float *end, char *text, char *altpath, char *altact)
{
    if(!cgui) return;
    Texture *t = path && *path ? textureload(path, 0, true, false) : NULL;
    if(t == notexture)
    {
        if(*altpath) t = textureload(altpath, 0, true, false);
        if(t == notexture) return;
    }
    int ret = cgui->slice(t, *scale, *start, *end, text && *text ? text : NULL);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", path, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "ui_image", true);
    }
}

void ui_text(char *name, char *icon, int *colour, int *icolour, int *wrap, int *faded, char *oicon, int *ocolor)
{
    if(cgui) cgui->text(name, *colour >= 0 ? *colour : 0xFFFFFF, *icon ? icon : NULL, *icolour >= 0 ? *icolour : 0xFFFFFF, *wrap > 0 ? *wrap : -1, *faded >= 0 ? *faded > 0 : false, *oicon ? oicon : NULL, *ocolor >= 0 ? *ocolor : 0xFFFFFF);
}

void ui_tab(char *name)
{
    if(cgui) cgui->tab(name);
}

void ui_status(char *str, int *width)
{
    if(cgui) cgui->setstatus("%s", *width, str);
}

void ui_tooltip(char *str, int *width)
{
    if(cgui) cgui->settooltip("%s", *width, str);
}

void ui_bar(int *size, int *space, int *colour, int *border)
{
    if(cgui) cgui->separator(*size, *space, *colour, *border);
}

void ui_fill(int *colour, int *levels)
{
    if(cgui) cgui->fill(*colour, *levels);
}

void ui_outline(int *colour, int *levels, int *offsetx, int *offsety)
{
    if(cgui) cgui->outline(*colour, *levels, 0, *offsetx, *offsety);
}

void ui_background(int *colour1, float *blend1, int *colour2, float *blend2, int *skinborder, int *levels)
{
    if(cgui) cgui->background(*colour1, *blend1, *colour2, *blend2, *skinborder!=0, *levels);
}

void ui_strut(float *strut, int *alt)
{
    if(cgui)
    {
        if(*alt) cgui->strut(*strut);
        else cgui->space(*strut);
    }
}

void ui_spring(int *weight)
{
    if(cgui) cgui->spring(max(*weight, 1));
}

void ui_visible(uint *body)
{
    if(cgui && cgui->visible()) execute(body);
}

void ui_visible_tab(uint *body)
{
    if(cgui && cgui->visibletab()) execute(body);
}

void ui_font(char *font, uint *body)
{
    if(cgui)
    {
        cgui->pushfont(font);
        execute(body);
        cgui->popfont();
    }
}

int ui_font_width(char *font)
{
    float oldtextscale = curtextscale;
    curtextscale = 1;
    if(font && *font) pushfont(font);
    int width = FONTW;
    if(font && *font) popfont();
    curtextscale = oldtextscale;
    return width;
}

int ui_font_height(char *font)
{
    float oldtextscale = curtextscale;
    curtextscale = 1;
    if(font && *font) pushfont(font);
    int height = FONTH;
    if(font && *font) popfont();
    curtextscale = oldtextscale;
    return height;
}

int ui_text_width(char *text, char *font, int wrap)
{
    float oldtextscale = curtextscale;
    curtextscale = 1;
    if(font && *font) pushfont(font);
    int width = 0, height = 0;
    text_bounds(text, width, height, 0, 0, wrap > 0 ? wrap : -1, TEXT_NO_INDENT);
    if(font && *font) popfont();
    curtextscale = oldtextscale;
    return width;
}

int ui_text_height(char *text, char *font, int wrap)
{
    float oldtextscale = curtextscale;
    curtextscale = 1;
    if(font && *font) pushfont(font);
    int width = 0, height = 0;
    text_bounds(text, width, height, 0, 0, wrap > 0 ? wrap : -1, TEXT_NO_INDENT);
    if(font && *font) popfont();
    curtextscale = oldtextscale;
    return height;
}

template<class T> static void update_value(char *var, T val, char *onchange)
{
    ident *id = writeident(var);
    updatelater.add().schedule(id, val);
    if(onchange && *onchange) updatelater.add().schedule(onchange);
}

static int get_value_int(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return int(*id->storage.f);
        case ID_SVAR: return parseint(*id->storage.s);
        case ID_ALIAS: return id->getint();
        default: return 0;
    }
}

static float get_value_float(char *var)
{
    ident *id = readident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return *id->storage.f;
        case ID_SVAR: return parsefloat(*id->storage.s);
        case ID_ALIAS: return id->getfloat();
        default: return 0;
    }
}

static const char *get_value_string(char *var)
{
    ident *id = getident(var);
    if(!id) return "";
    switch(id->type)
    {
        case ID_VAR: return intstr(*id->storage.i);
        case ID_FVAR: return floatstr(*id->storage.f);
        case ID_SVAR: return *id->storage.s;
        case ID_ALIAS: return id->getstr();
        default: return "";
    }
}

void ui_progress(float *percent, float *scale)
{
    if(!cgui) return;
    cgui->progress(*percent, *scale);
}

void ui_slider(char *var, int *min, int *max, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui || !var || !*var) return;
    int oldval = get_value_int(var), val = oldval, vmin = *max > INT_MIN ? *min : getvarmin(var), vmax = *max > INT_MIN ? *max : getvarmax(var);
    if(vmax >= INT_MAX-1)
    { // not a sane value for a slider..
        int vdef = getvardef(var);
        vmax = vdef > vmin ? vdef*3 : vmin*4;
    }
    cgui->slider(val, vmin, vmax, *colour >= 0 ? *colour : 0xFFFFFF, NULL, *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(val != oldval) update_value(var, val, onchange);
}

void ui_list_slider(char *var, char *list, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = get_value_int(var), index = vals.find(val), offset = index;
    cgui->slider(offset, 0, vals.length()-1, *colour >= 0 ? *colour : 0xFFFFFF, intstr(val), *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(offset != index) update_value(var, vals[offset], onchange);
}

void ui_name_slider(char *var, char *names, char *list, char *onchange, int *reverse, int *scroll, int *colour, int *style, int *scolour)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = get_value_int(var), index = vals.find(val), offset = index;
    char *label = indexlist(names, offset);
    cgui->slider(offset, 0, vals.length()-1, *colour >= 0 ? *colour : 0xFFFFFF, label, *reverse ? true : false, *scroll ? true : false, *style, *scolour);
    if(offset != index) update_value(var, vals[offset], onchange);
    delete[] label;
}

void ui_checkbox(char *name, char *var, float *on, float *off, char *onchange, int *colour)
{
    if(!cgui) return;
    bool enabled = get_value_float(var) != *off, two = getfvarmax(var) == 2, next = two && get_value_float(var) == 1.0f;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, "checkbox", 0xFFFFFF, -1, true, enabled ? "checkboxon" : NULL, enabled && two && !next ? ui_color_checkbox_two : ui_color_checkbox);
    if(ret&GUI_UP) update_value(var, enabled ? (two && next ? 2.0f : *off) : (*on || *off ? *on : 1.0f), onchange);
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "checkbox", true);
    }
}

void ui_radiobutton(char *name, char *var, float *n, char *onchange, int *colour)
{
    if(!cgui) return;
    bool enabled = get_value_float(var) == *n;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, "radiobox", *colour >= 0 ? *colour : 0xFFFFFF, -1, true, enabled ? "radioboxon" : NULL, ui_color_radiobutton_box);
    if(ret&GUI_UP)
    {
        if(!enabled) update_value(var, *n, onchange);
    }
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "radio", true);
    }
}

void ui_bitfield(char *name, char *var, int *mask, char *onchange, int *colour)
{
    if(!cgui) return;
    int val = get_value_int(var);
    bool enabled = (val & *mask) != 0;
    int ret = cgui->button(name, *colour >= 0 ? *colour : 0xFFFFFF, "checkbox", *colour >= 0 ? *colour : 0xFFFFFF, -1, true, enabled ? "checkboxon" : NULL, ui_color_checkbox);
    if(ret&GUI_UP) update_value(var, enabled ? val & ~*mask : val | *mask, onchange);
    else if(ret&GUI_ROLLOVER)
    {
        setsvar("guirollovername", name, true);
        setsvar("guirolloveraction", var, true);
        setsvar("guirollovertype", "bitfield", true);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void ui_textfield(char *var, int *maxlength, char *onchange, int *colour, int *focus, char *parent, int *height, char *prompt, int *immediate)
{
    if(!cgui) return;
    const char *initval = get_value_string(var);
    char *result = cgui->field(var, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, *height, initval, EDITORFOCUSED, *focus!=0, parent, prompt, *immediate!=0);
    if(result && (!*immediate || strcmp(initval, result))) update_value(var, result, onchange);
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void ui_editor(char *name, int *maxlength, int *height, int *mode, int *colour, int *focus, char *parent, char *str, char *prompt)
{
    if(!cgui) return;
    cgui->field(name, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : 12, *height, str && *str ? str : NULL, *mode<=0 ? EDITORFOREVER : *mode, *focus!=0, parent, prompt);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void ui_keyfield(char *var, int *maxlength, char *onchange, int *colour, int *focus, char *parent, char *prompt, int *immediate)
{
    if(!cgui) return;
    const char *initval = get_value_string(var);
    char *result = cgui->keyfield(var, *colour >= 0 ? *colour : 0xFFFFFF, *maxlength ? *maxlength : -8, 0, initval, EDITORFOCUSED, *focus!=0, parent, prompt, *immediate!=0);
    if(result && (!*immediate || strcmp(initval, result))) update_value(var, result, onchange);
}

//use text<action> to do more...

void ui_body(uint *contents, char *action, char *altact, uint *onhover)
{
    if(!cgui) return;
    cgui->pushlist(action && *action ? true : false);
    execute(contents);
    int ret = cgui->poplist();
    if(guilayoutpass) return;
    bool active = cgui->unique_object_active(ret & GUI_ROLLOVER);
    if(ret&GUI_UP && active)
    {
        char *act = NULL;
        if(ret&GUI_ALT && altact && *altact) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER) execute(onhover);
}

void ui_list(uint *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    execute(contents);
    cgui->poplist();
}

void newgui(char *name, char *contents, char *initscript)
{
    menu *m = menus.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &menus[name];
        m->name = name;
    }
    else
    {
        DELETEA(m->header);
        freecode(m->contents);
        freecode(m->init_script);
    }
    m->contents = contents && *contents ? compilecode(contents) : NULL;
    m->init_script = initscript && *initscript ? compilecode(initscript) : NULL;
}

void ui_header(char *name)
{
    if(!cmenu) return;
    DELETEA(cmenu->header);
    cmenu->header = name && *name ? newstring(name) : NULL;
}

void ui_modify(char *name, char *contents)
{
    menu *m = menus.access(name);
    if(!m) return;
    freecode(m->contents);
    m->contents = contents && *contents ? compilecode(contents) : NULL;
}

COMMAND(0, newgui, "sss");
COMMAND(0, ui_header, "s");
COMMAND(0, ui_modify, "ss");
COMMAND(0, ui_button, "ssssbbbbsb");
COMMAND(0, ui_text, "ssbbbbsb");
COMMANDN(0, cleargui, cleargui_, "i");
ICOMMAND(0, showgui, "si", (const char *s, int *n), showgui(s, *n));
COMMAND(0, ui_show_title, "i");
COMMAND(0, ui_show_bg_fx, "i");
COMMAND(0, ui_stay_open, "e");
COMMAND(0, ui_no_hit_fx, "e");
COMMAND(0, ui_no_cursor_fx, "e");
COMMAND(0, ui_no_skin_fx, "e");

COMMAND(0, ui_list, "e");
COMMAND(0, ui_body, "esse");
COMMAND(0, ui_bar, "iibb");
COMMAND(0, ui_fill, "ii");
COMMAND(0, ui_outline, "iiii");
COMMAND(0, ui_background, "bgbgii");
COMMAND(0, ui_strut, "fi");
COMMAND(0, ui_spring, "i");
COMMAND(0, ui_visible, "e");
COMMAND(0, ui_visible_tab, "e");
COMMAND(0, ui_font, "se");
COMMAND(0, ui_image, "ssfissbsb");
COMMAND(0, ui_slice, "ssfffsss");
COMMAND(0, ui_progress, "ff");
COMMAND(0, ui_slider, "sbbsiibib");
COMMAND(0, ui_list_slider, "sssiibib");
COMMAND(0, ui_name_slider, "ssssiibib");
COMMAND(0, ui_radiobutton, "ssfsb");
COMMAND(0, ui_bitfield, "ssisb");
COMMAND(0, ui_checkbox, "ssffsb");
COMMAND(0, ui_tab, "s");
COMMAND(0, ui_status, "si");
COMMAND(0, ui_tooltip, "si");
COMMAND(0, ui_textfield, "sisbisisi");
COMMAND(0, ui_keyfield, "sisbissi");
COMMAND(0, ui_editor, "siiibisss");

ICOMMAND(0, guicount, "", (), intret(menustack.length()));
ICOMMAND(0, ui_font_width, "s", (char * font), intret(ui_font_width(font)));
ICOMMAND(0, ui_font_height, "s", (char * font), intret(ui_font_height(font)));
ICOMMAND(0, ui_text_width, "ssb", (char * text, char * font, int * wrap), intret(ui_text_width(text, font, *wrap)));
ICOMMAND(0, ui_text_height, "ssb", (char * text, char * font, int * wrap), intret(ui_text_height(text, font, *wrap)));

void ui_player_preview(int *model, int *color, int *team, int *weap, char *vanity, char *action, float *scale, int *overlaid, float *size, float *blend, char *altact)
{
    if(!cgui) return;
    int ret = cgui->playerpreview(*model, *color, *team, *weap, vanity, *scale, *overlaid!=0, *size!=0 ? *size : 1.f, *blend >= 0 ? *blend : 1.f);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        defformatstring(str, "%d %d %d %d %s", *model, *color, *team, *weap, vanity);
        setsvar("guirollovername", str, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "player", true);
    }
}
COMMAND(0, ui_player_preview, "iiiissfifgs");

void ui_model_view(char *model, char *animspec, char *action, float *scale, int *overlaid, float *size, float *blend, char *altact)
{
    if(!cgui) return;
    int anim = ANIM_ALL;
    if(animspec && *animspec)
    {
        if(isdigit(animspec[0]))
        {
            anim = parseint(animspec);
            if(anim >= 0) anim %= ANIM_INDEX;
            else anim = ANIM_ALL;
        }
        else
        {
            vector<int> anims;
            game::findanims(animspec, anims);
            if(anims.length()) anim = anims[0];
        }
    }
    int ret = cgui->modelpreview(model, anim|ANIM_LOOP, *scale, *overlaid!=0, *size!=0 ? *size : 1.f, *blend!=0 ? *blend : 1.f);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        defformatstring(str, "%s [%s]", model, animspec);
        setsvar("guirollovername", str, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "model", true);
    }
}
COMMAND(0, ui_model_view, "sssfiffs");

void ui_prefab_preview(char *prefab, int *color, char *action, float *scale, int *overlaid, char *altact)
{
    if(!cgui) return;
    int ret = cgui->prefabpreview(prefab, vec::hexcolor(*color), *scale, *overlaid!=0);
    if(ret&GUI_UP)
    {
        char *act = NULL;
        if(altact && *altact && ret&GUI_ALT) act = altact;
        else if(action && *action) act = action;
        if(act)
        {
            updatelater.add().schedule(act);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&GUI_ROLLOVER)
    {
        defformatstring(str, "%s %d", prefab, *color);
        setsvar("guirollovername", str, true);
        setsvar("guirolloveraction", action, true);
        setsvar("guirollovertype", "prefab", true);
    }
}
COMMAND(0, ui_prefab_preview, "sisfis");

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

static struct applymenu : menu
{
    void gui(guient &g, bool firstpass)
    {
        if(menustack.empty()) return;
        g.start(menu_start, NULL, true);
        g.text("the following settings have changed:");
        g.pushfont("little");
        loopv(needsapply) g.text(needsapply[i].desc, 0xFFFFFF, "point");
        g.popfont();
        g.separator();
        g.text("apply changes now?");
        g.pushlist();
        g.spring();
        if(g.button("\fgOK")&GUI_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
            if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
            if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
            clearlater = true;
        }
        g.spring();
        if(g.button("\focancel")&GUI_UP) clearlater = true;
        g.spring();
        g.poplist();
        g.end();
    }

    void clear()
    {
        needsapply.shrink(0);
    }
} applymenu;

VAR(IDF_PERSIST, applydialog, 0, 1, 1);

void addchange(const char *desc, int type, bool force)
{
    if(!applydialog || force)
    {
        int changetypes = type;
        if(menustack.find(&applymenu) >= 0)
        {
            loopv(needsapply) changetypes |= needsapply[i].type;
            clearlater = true;
        }
        if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
        if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
    }
    else
    {
        loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
        needsapply.add(change(type, desc));
        if(needsapply.length() && menustack.find(&applymenu) < 0)
            push_ui(&applymenu, max(menustack.length() - 1, 0));
    }
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) remove_ui(&applymenu);
}

void menuprocess()
{
    int level = menustack.length();
    interactive = true;
    loopv(updatelater) updatelater[i].run();
    updatelater.shrink(0);
    interactive = false;
    if(clearlater)
    {
        if(level==menustack.length()) loopi(level) pop_ui();
        clearlater = false;
    }
}

void menu_progress()
{
    menu *m = menus.access("loading");
    if(m)
    { // it doesn't need to exist
        m->use_title = m->use_input = m->world = false;
        m->built_in = true;
        UI::addcb(m);
    }
}

void menu_main()
{
    if(!menustack.empty()) UI::addcb(menustack.last());
}

bool menuactive()
{
    return !menustack.empty();
}

ICOMMAND(0, menustacklen, "", (void), intret(menustack.length()));

void guiirc(const char *s, int width, int height)
{
    extern bool ircgui(guient *g, const char *s, int width, int height);
    if(cgui)
    {
        if(!ircgui(cgui, s, width > 0 ? width : 100, height > 0 ? height : 25) && shouldclearmenu) clearlater = true;
    }
}
ICOMMAND(0, ircgui, "sii", (char *s, int *w, int *h), guiirc(s, *w, *h));

void ui_console(int width, int height, const char *init)
{
    extern bool consolegui(guient *g, int width, int height, const char *init, int &update);
    static int consoleupdate = -1;
    if(cgui)
    {
        if(!consolegui(cgui, width > 0 ? width : 100, height > 0 ? height : 25, init && *init ? init : "/", consoleupdate) && shouldclearmenu)
        {
            clearlater = true;
            consoleupdate = -1;
        }
    }
}
ICOMMAND(0, consolegui, "iis", (int *w, int *h, char *i), ui_console(*w, *h, i));
