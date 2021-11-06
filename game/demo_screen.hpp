#pragma once

#include <core/event.hpp>

#include <string>
#include <string_view>

namespace core
{
    class game_data;
}

namespace grfx
{
    class system;
}

namespace game
{
    class demo_screen
    {
    public:
        explicit demo_screen(const std::string_view title) : title_(title) {}

        void draw(grfx::system& gfx, core::game_data& data) const;
        void handle_event(const core::event_t& e) {}
        void tick() {}

    private:
        std::string title_;
    };

}