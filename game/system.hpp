#pragma once

#include <core/event.hpp>
#include <core/game_data.hpp>
#include <game/demo_screen.hpp>
#include <game/arena.hpp>

#include <variant>

namespace grfx
{
    class system;
}

namespace rndr
{
    class system;
}

namespace game
{
    using game_state = std::variant<demo_screen, arena>;

    class system
    {
    public:
        system(const rndr::system& renderer, core::game_data& data, const core::iwad_description& iwad);

        void new_game(const start_parameters& p);

        void draw(grfx::system& gfx, core::game_data& data) const;

        void handle_event(const core::event_t& e);

        void tick();

    private:
        const rndr::system& renderer_;
        core::game_data& data_;
        core::iwad_description iwad_;
        start_parameters parameters_;
        game_state state_ = demo_screen("TITLEPIC");
    };
}