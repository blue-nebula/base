#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <deque>
#include <vector>
#include <map>
#include <array>

static const int MAX_HISTORY = 1000;
static const int MAX_INPUT_HISTORY = 100;

class Console;

////////////////////////
/// COMPLETIONS BASE ///
////////////////////////

class CompletionEntryBase
{
public:
    int completion_length = 0;

    virtual std::string get_icon() { return ""; };
    virtual int get_icon_color() { return 0; };
    virtual std::string get_title() { return ""; };
    virtual std::string get_description() { return ""; };
};

class CompletionEngineBase
{
public:
    int scroll_pos = 0;
    int max_entries_per_page = 5;

    virtual int stick_to_buffer_idx() { return 0; };
    virtual bool can_complete(Console& console) { return false; };
    virtual std::vector<CompletionEntryBase*> get_completions(const std::string buffer) { return std::vector<CompletionEntryBase*>(); }; 
    virtual void select_entry(CompletionEntryBase* entry, Console& console) { };
};


/////////////////////
/// INPUT HISTORY ///
/////////////////////
struct InputHistoryLine
{
    std::string text;
    std::string icon;
    int action;
};

class InputHistory
{
public:
    std::deque<InputHistoryLine> history;
    InputHistoryLine current_line;
    int hist_pos = -1;

    bool scroll(const int lines);
    void save(InputHistoryLine line);
};

///////////////////////
/// CONSOLE HISTORY ///
///////////////////////
struct ConsoleMessage
{
    std::string text = "";
    std::vector<std::string> lines;
    int type;
    
    int reftime;
    int out_time;
    int real_time;
    
    int get_num_lines();

    bool read = false;
};

class History
{
private:
    int line_width = 0;
    int scroll_pos = 0;
    int scroll_info_hist_idx = 0;
    int scroll_info_line_idx = 0;
    bool scroll_info_outdated = false; 
    int num_linebreaks = 0;
    int unread_messages = 0;
    int max_num_messages = 1000;

    void calculate_wordwrap(ConsoleMessage& msg);
    void calculate_all_wordwraps();

    void recalc_scroll_info();
    std::vector<int> type_filter; 

public:
    History();
    History(std::vector<int> type_filter);

    std::deque<ConsoleMessage> h;

    std::map<const int, std::pair<int, float>> type_background_colors; 

    bool accepts_type(int type);

    int get_num_lines();
    void set_line_width(int w);
    //TODO: replace std::pair with smth more fitting
    std::pair<int, int> get_relative_line_info(int n, int hist_idx, int line_idx);

    void set_max_num_messages(const int max_messages);
    int get_scroll_pos();
    void reset_scroll();
    std::array<int, 2> get_scroll_info();
    bool scroll(const int lines);
    
    ConsoleMessage last_message_of_type(int type);

    void mark_all_read();
    void read_message(int idx);
    int get_num_unread_messages();
    void remove_message(const int idx);

    int missed_lines = 0;

    void clear();
    void save(ConsoleMessage& msg);
};

///////////////
/// CONSOLE ///
///////////////
class Console
{
private:
    std::string buffer;
    int curr_action;
    std::string curr_icon; 
    std::vector<CompletionEngineBase*> completion_engines; 
    std::vector<CompletionEntryBase*> curr_completions;

    /// KEY ACTIONS ///
    ///////////////////

    void cursor_jump_to_buffer_start();
    void cursor_jump_to_buffer_end();
    void cursor_move_left();
    void cursor_move_right();
    void buffer_delete_at_cursor();
    void buffer_remove_at_cursor();

    bool open = false;
    int cursor_pos = -1;

    int completion_scroll_pos = 0;
    int completion_selection_idx = 0;
    int curr_completion_engine = -1;
    int completion_lines_per_view = 4;
    InputHistory input_history;
public:
    Console();

    std::map<const int, std::array<short, 3>> type_fade_times; 

    enum
    {
        MODE_NONE =    0,
        MODE_SEARCH =  1,
        MODE_COMMAND = 2,
    };

    int get_mode();

    // constants
    const int get_page_size();
    const char command_prefix = '/';
    const int max_buffer_len = 4096;

    // buffer
    void set_buffer(std::string text);
    void set_input(std::string init = "", int action = 0, std::string icon = "");
    std::string get_buffer();
    void insert_in_buffer(const std::string text);
    void run_buffer();

    // opened/closed state
    bool is_open(); 
    void open_console();
    void close_console(); 
    
    std::string get_icon();
    int get_icon_color();
    int get_cursor_pos();
 
    // history
    enum
    {
        HIST_CHAT =    0,
        HIST_CONSOLE = 1,
        HIST_PREVIEW = 2,
        HIST_MAX
    };

    std::map<const int, History> histories;

    int selected_hist = HIST_CHAT;
    History& curr_hist();
    void clear_curr_hist();
    void print(int type, const std::string text);

    // info bar
    std::string get_info_bar_text();

    // input/key processing
    bool process_key(int code, bool isdown);
    bool process_text_input(const char* str, int len);

    // Completion //
    bool completion_scroll(const int lines);
    int get_completion_scroll_pos();
    int get_completion_selection();
    int get_completion_lines_per_view();
    void register_completion(CompletionEngineBase* completion);
    std::vector<CompletionEntryBase*> get_curr_completions();
    CompletionEngineBase* get_curr_completion_engine();


    // others
    void set_max_line_width(int w);
    int get_say_text_color();
    int unseen_error_messages = -1;
};

#endif //CONSOLE_H
