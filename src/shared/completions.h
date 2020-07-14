#ifndef COMPLETIONS_H
#define COMPLETIONS_H

#include <vector>
#include <string>
#include "cube.h"

namespace completion
{
    class Completion
    {
    public:
        Completion(ident id) : id(id), type(IDENTIFIER) {}
        Completion(std::string text) : text(text), type(OTHER) {}

        enum completion_type {
            IDENTIFIER = 0, OTHER
        };

        ident id;
        std::string text;
        completion_type type;

        std::string get_ident_flags_text();
        std::string get_ident_args_text();
        void get_ident_values(std::string (&values)[4]);
        bool is_ident_bool();
    };

    extern std::string completed_word;
    extern std::vector<Completion> available_completions;
    extern int completion_pos;
    extern int selected_completion;
    const int max_completions = 5;

    extern bool completions_available();
    extern bool is_command(const std::string& text);
    extern bool could_contain_completion(const std::string& text);
    extern size_t get_name_pos(const std::string& text);

    extern bool move_completion_select_up();
    extern bool move_completion_select_down();
    extern bool selected_completion_is_available();
    extern void get_completions(const std::string& text);
};

#endif // COMPLETIONS_H
