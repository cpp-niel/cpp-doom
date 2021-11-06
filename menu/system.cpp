#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
#pragma ide diagnostic ignored "hicpp-named-parameter"
#include <menu/system.hpp>

#include <core/english.hpp>
#include <doomkeys.hpp>
#include <grfx/system.hpp>
#include <menu/draw_text.hpp>
#include <stdx/overload.hpp>

#include <numeric>

namespace menu
{
    namespace
    {
        using namespace ::std::string_view_literals;

        constexpr auto line_height = 16;
        constexpr auto skull_x_offset = -32;

        constexpr auto skull_names = std::array{"M_SKULL1"sv, "M_SKULL2"sv};

        int string_width(grfx::system& gfx, const std::string_view text)
        {
            return std::accumulate(text.begin(), text.end(), 0, [&](const int acc, const char chr) {
                const auto c = core::index_in_hud_font(chr);
                return acc + (core::is_in_hud_font(c) ? gfx.hud_font_patch(c).width : 4);
            });
        }

        void placeholder(system&, int) {}

        void read_this_1(system& sys, int);
        void read_this_2(system& sys, int);
        void finish_read_this(system& sys, int);
        void choose_episode(system& sys, int);
        void choose_skill(system& sys, int);
        void new_game(system& sys, int);

        void quit(system&, int key)
        {
            if (key != 'y') return;

            // todo play sound
            exit(0);
        }

        void quit_doom(system& sys, int)
        {
            const auto text = std::string(ENDGAME);
            sys.message(text, quit, true);
        }

        void draw_main_menu(grfx::system& gfx, core::game_data& data)
        {
            gfx.draw_patch(94, 2, *core::cache_lump<grfx::patch_t>(data, "M_DOOM"));
        }

        void draw_help_page_1(grfx::system& gfx, core::game_data& data)
        {
            gfx.draw_patch(0, 0, *core::cache_lump<grfx::patch_t>(data, "HELP2"));
        }

        void draw_help_page_2(grfx::system& gfx, core::game_data& data)
        {
            gfx.draw_patch(0, 0, *core::cache_lump<grfx::patch_t>(data, "HELP1"));
        }

        void draw_choose_episode_page(grfx::system& gfx, core::game_data& data)
        {
            gfx.draw_patch(54, 38, *core::cache_lump<grfx::patch_t>(data, "M_EPISOD"));
        }

        auto main_page = menu_page{
            .prev_menu = nullptr,
            .items = {{.status = entry_status::ok, .name = "M_NGAME", .routine = new_game, .hot_key = 'n'},
                      {.status = entry_status::ok, .name = "M_OPTION", .routine = placeholder, .hot_key = 'o'},
                      {.status = entry_status::ok, .name = "M_LOADG", .routine = placeholder, .hot_key = 'l'},
                      {.status = entry_status::ok, .name = "M_SAVEG", .routine = placeholder, .hot_key = 's'},
                      {.status = entry_status::ok, .name = "M_RDTHIS", .routine = read_this_1, .hot_key = 'r'},
                      {.status = entry_status::ok, .name = "M_QUITG", .routine = quit_doom, .hot_key = 'q'}},
            .draw = draw_main_menu,
            .x = 97,
            .y = 64,
            .last_on = 0};

        auto help_page_1 = menu_page{.prev_menu = &main_page,
                                     .items = {{.status = entry_status::ok, .routine = read_this_2}},
                                     .draw = draw_help_page_1,
                                     .x = 280,
                                     .y = 185,
                                     .last_on = 0};

        auto help_page_2 = menu_page{.prev_menu = &help_page_1,
                                     .items = {{.status = entry_status::ok, .routine = finish_read_this}},
                                     .draw = draw_help_page_2,
                                     .x = 330,
                                     .y = 175,
                                     .last_on = 0};

        auto choose_episode_page = menu_page{
            .prev_menu = &main_page,
            .items =
                {
                    {.status = entry_status::ok, .name = "M_EPI1", .routine = choose_episode, .hot_key = 'k'},
                    {.status = entry_status::ok, .name = "M_EPI2", .routine = choose_episode, .hot_key = 't'},
                    {.status = entry_status::ok, .name = "M_EPI3", .routine = choose_episode, .hot_key = 'i'},
                    //{.status = entry_status::ok, .name = "M_EPI4", .routine = choose_episode, .hot_key = 't'},
                },
            .draw = draw_choose_episode_page,
            .x = 48,
            .y = 63,
            .last_on = 0};

        auto new_game_page =
            menu_page{.prev_menu = &choose_episode_page,
                      .items =
                          {
                              {.status = entry_status::ok, .name = "M_JKILL", .routine = choose_skill, .hot_key = 'i'},
                              {.status = entry_status::ok, .name = "M_ROUGH", .routine = choose_skill, .hot_key = 'h'},
                              {.status = entry_status::ok, .name = "M_HURT", .routine = choose_skill, .hot_key = 'h'},
                              {.status = entry_status::ok, .name = "M_ULTRA", .routine = choose_skill, .hot_key = 'u'},
                              {.status = entry_status::ok, .name = "M_NMARE", .routine = choose_skill, .hot_key = 'n'},
                          },
                      .draw = draw_choose_episode_page,
                      .x = 48,
                      .y = 63,
                      .last_on = 2};

        void read_this_1(system& sys, int) { sys.switch_to_page(&help_page_1); }

        void read_this_2(system& sys, int) { sys.switch_to_page(&help_page_2); }

        void finish_read_this(system& sys, int) { sys.switch_to_page(&main_page); }

        void choose_episode(system& sys, int choice)
        {
            if ((sys.game_mode() == core::game_mode::shareware) and (choice != 0))
            {
                sys.message(SWSTRING, nullptr, false);
                sys.switch_to_page(&help_page_1);
                return;
            }

            if ((sys.game_mode() == core::game_mode::registered) and (choice > 2))
            {
                fmt::print(stderr, "4th episode requires UltimateDOOM\n");
                choice = 0;
            }

            sys.set_episode(choice);
            sys.switch_to_page(&new_game_page);
        }

        void verify_nightmare(system& sys, const int key)
        {
            if (key != 'y') return;

            sys.new_game(new_game_page.items.size() - 1, 1);
        }

        void choose_skill(system& sys, int skill)
        {
            if (skill == new_game_page.items.size() - 1)
            {
                sys.message(NIGHTMARE, verify_nightmare, true);
                return;
            }

            sys.new_game(skill, 2);
        }

        void new_game(system& sys, int choice)
        {
            if (sys.game_mode() == core::game_mode::commercial)
                sys.switch_to_page(&new_game_page);
            else
                sys.switch_to_page(&choose_episode_page);
        }
    }

    system::system(const core::iwad_description& iwad, game::system& game_sys)
        : game_(game_sys), iwad_(iwad), current_page_(&main_page)
    {
    }

    void system::draw(grfx::system& gfx, core::game_data& data) const
    {
        if (current_message_.has_value())
        {
            const auto lines = std::ranges::split_view(current_message_->text, '\n');
            const auto line_height = gfx.hud_font_patch(core::hud_font_start).height;
            const auto height = std::ranges::distance(lines) * line_height;
            for (int y = grfx::original_screen_height / 2 - height / 2; const auto& line_chars : lines)
            {
                const auto line =
                    std::string_view(&(*std::ranges::begin(line_chars)), std::ranges::distance(line_chars));
                const auto x = grfx::original_screen_width / 2 - string_width(gfx, line) / 2;
                draw_text(gfx, x, y, line);
                y += line_height;
            }

            return;
        }

        if (!is_active_) return;

        current_page_->draw(gfx, data);

        for (auto y = current_page_->y; const auto& item : current_page_->items)
        {
            if (!item.name.empty())
                gfx.draw_patch(current_page_->x, y, *core::cache_lump<grfx::patch_t>(data, item.name));

            y += line_height;
        }

        gfx.draw_patch(current_page_->x + skull_x_offset, current_page_->y - 5 + selected_item_index_ * line_height,
                       *core::cache_lump<grfx::patch_t>(data, skull_names[current_skull_]));
    }

    void system::message(const std::string_view text, void (*routine)(system&, int), bool is_input_required)
    {
        current_message_ =
            message_description{.text = std::string(text), .routine = routine, .is_input_required = is_input_required};
    }

    void system::new_game(const int skill, const int map)
    {
        is_active_ = false;
        game_.new_game({.skill = skill, .episode = episode_, .map = map});
    }

    bool system::handle_event(const core::event_t& e)
    {
        return std::visit(stdx::overload{[&](const core::key_down_event& e) { return handle_key_down_event(e); },
                                         [&](const core::key_up_event& e) { return handle_key_up_event(e); },
                                         [&](const core::quit_event& e) { return handle_quit_event(e); }},
                          e);
    }

    bool system::handle_key_down_event(const core::key_down_event& e)
    {
        if (current_message_)
        {
            if (current_message_->is_input_required)
            {
                const auto cancel_keys = std::array<char, 4>{' ', KEY_ESCAPE, 'y', 'n'};
                if (std::ranges::find(cancel_keys, e.us_key_code) == cancel_keys.end()) { return false; }
            }

            if (current_message_->routine) current_message_->routine(*this, e.us_key_code);

            current_message_.reset();
            // todo play sound
            return true;
        }

        if (!is_active_) return false;

        if (e.us_key_code == KEY_DOWNARROW)
        {
            if (selected_item_index_ + 1 > std::ssize(current_page_->items) - 1)
                selected_item_index_ = 0;
            else
                selected_item_index_++;
            // todo play sound

            return true;
        }

        if (e.us_key_code == KEY_UPARROW)
        {
            if (selected_item_index_ == 0)
                selected_item_index_ = std::ssize(current_page_->items) - 1;
            else
                selected_item_index_--;
            // todo play sound

            return true;
        }

        if (e.us_key_code == KEY_ENTER)
        {
            if (current_page_->items[selected_item_index_].status == entry_status::ok)
            {
                current_page_->last_on = selected_item_index_;
                if (current_page_->items[selected_item_index_].status == entry_status::arrows_ok)
                {
                    current_page_->items[selected_item_index_].routine(*this, 1);  // right arrow
                    // todo play sound
                }
                else
                {
                    current_page_->items[selected_item_index_].routine(*this, selected_item_index_);
                    // todo play sound
                }
            }
            return true;
        }

        return false;
    }

    bool system::handle_key_up_event(const core::key_up_event&) { return false; }

    bool system::handle_quit_event(const core::quit_event&)
    {
        if (is_active_ && current_message_.has_value() && current_message_->routine == quit) { quit(*this, 'y'); }
        else
        {
            // todo play sound
            quit_doom(*this, 0);
        }

        return true;
    }

    void system::switch_to_page(menu_page* page)
    {
        current_page_ = page;
        selected_item_index_ = current_page_->last_on;
    }
}

#pragma clang diagnostic pop