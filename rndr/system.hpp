#pragma once

#include <core/game_data.hpp>

#include <memory>

namespace game
{
    struct level_t;
    struct mobj_t;
}

namespace grfx
{
    class system;
}

namespace rndr
{
    class system
    {
    public:
        explicit system(core::game_data& data);
        ~system();

        void draw(const game::level_t& level, const game::mobj_t& player, core::game_data& data) const;

        [[nodiscard]] int texture_num(const std::string& name) const;

    private:
        struct impl;
        std::unique_ptr<impl> impl_;
    };
}