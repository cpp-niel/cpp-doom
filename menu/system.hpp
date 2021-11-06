#pragma once

#include <core/event.hpp>
#include <core/game_data.hpp>
#include <game/system.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace grfx
{
    class patch_t;
    class system;
}

namespace menu
{
    class system;

    enum class entry_status
    {
        no_cursor,
        ok,
        arrows_ok
    };

    struct menu_entry
    {
        entry_status status;

        std::string_view name;

        // choice = menu item #.
        // if status = 2,
        //   choice=0:leftarrow,1:rightarrow
        void (*routine)(system&, int);
        char hot_key;
    };

    struct menu_page
    {
        menu_page* prev_menu;
        std::vector<menu_entry> items;
        void (*draw)(grfx::system&, core::game_data&);
        short x;
        short y;        // x,y of menu
        short last_on;  // last item user was on in menu
    };

    struct message_description
    {
        std::string text;
        int x = 0;
        int y = 0;
        void (*routine)(system&, int);
        bool is_input_required = false;
    };

    class system
    {
    public:
        explicit system(const core::iwad_description& iwad, game::system& game_sys);

        void draw(grfx::system& gfx, core::game_data& data) const;

        bool handle_event(const core::event_t& e);

        void message(const std::string_view text, void (*routine)(system&, int), bool is_input_required);

        void switch_to_page(menu_page* page);

        [[nodiscard]] auto game_mode() const { return iwad_.mode; }

        void set_episode(const int e) { episode_ = e + 1; }
        void new_game(const int skill, const int map);

    private:
        game::system& game_;
        core::iwad_description iwad_;
        bool is_active_ = true;
        menu_page* current_page_ = nullptr;
        short selected_item_index_ = 0;
        int current_skull_ = 0;
        std::optional<message_description> current_message_;

        int episode_ = 0;

        bool handle_key_down_event(const core::key_down_event& e);
        bool handle_key_up_event(const core::key_up_event& e);
        bool handle_quit_event(const core::quit_event& e);
    };
}
