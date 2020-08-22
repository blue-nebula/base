#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <deque>
#include <vector>

static const int MAX_HISTORY = 1000;
static const int MAX_INPUT_HISTORY = 100;

// behaves like normal queue, except when pushing to a "full" queue, the oldest element
// gets popped
/*
template <typename T, int max_len, typename Container=std::queue<T>>
class FixedQueue : public std::deque<T, Container> 
{
public:
    void push_front(const T& value)
    {
        if (this->size() == max_len)
        {
            this->c.pop_back();
        }
        std::deque<T, Container>::push_front(value);
    }
};
*/

class ConsoleLine
{
public:
    std::string text;
    std::vector<std::string> lines;
    std::string raw_text;
    int type;
    int reftime;
    int out_time;
    int real_time;
    int num_linebreaks = 0;

    bool hide = false;
};

class History
{
public:
    std::deque<ConsoleLine> h;

    int num_linebreaks = 0;
    int scroll_pos = 0;
    int scroll_info_hist_idx = 0;
    int scroll_info_line_idx = 0;
    bool scroll_info_outdated = false;

    bool move(int lines);
    void recalc_scroll_info();
    //TODO: replace std::pair with smth more fitting
    std::pair<int, int> get_relative_line_info(int n, int hist_idx, int line_idx);
    void save(ConsoleLine line);
};

class InputHistoryLine
{
public:
    std::string text;
    int icon;
    int action;
};

class InputHistory
{
public:
    std::deque<InputHistoryLine> history;
    InputHistoryLine current_line;
    int current_history_pos = -1;

    bool go_up();
    bool go_down();
    void save(InputHistoryLine line);
};

class Console
{
private:
    std::string buffer;
public:
    enum
    {
        MODE_NONE =    0,
        MODE_SEARCH =  1,
        MODE_COMMAND = 2,
    };

    // constants
    const char command_prefix = ':';
    const char search_prefix = '/';
    const int max_buffer_len = 4096;

    void set_buffer(std::string text);
    std::string get_buffer();
    int cursor_pos = -1;

    /////////////////
    /// HISTORIES ///
    /////////////////
    InputHistory input_history;
    
    enum
    {
        HIST_ALL =     0,
        HIST_CHAT =    1,
        HIST_CONSOLE = 2,
    };

    History all_history;   
    History chat_history; 
    History console_history;

    int selected_hist = HIST_CHAT;
    History& curr_hist();

    // info bar
    std::string get_info_bar_text();

    void print(int type, const std::string text, const std::string raw_text = "");
    void open_console();
    void close_console();
    void insert_in_buffer(const std::string text);

    bool process_key(int code, bool isdown);
    bool process_text_input(const char* str, int len);

    void run_buffer();
    int get_mode();

    //////////////
    /// SEARCH ///
    //////////////

    std::vector<std::string> search_results;

    // updates the search results, should be called semi-automatically to save performance
    void update_search();
};

#endif //CONSOLE_H
