#pragma once

#include <core/event.hpp>

#include <memory>

namespace core
{
    class game_data;
    class iwad_description;
}

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
    struct start_parameters
    {
        int skill = 0;
        int episode = 0;
        int map = 0;
    };

    class arena
    {
    public:
        explicit arena(const core::iwad_description& iwad, const start_parameters& parameters, core::game_data& data,
                       const rndr::system& renderer);
        ~arena();

        void draw(grfx::system& gfx, core::game_data& data) const;
        void handle_event(const core::event_t& e);
        void tick();

        struct impl;

    private:
        std::unique_ptr<impl> impl_;
    };
}