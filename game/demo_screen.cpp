#include <game/demo_screen.hpp>

#include <core/game_data.hpp>
#include <grfx/system.hpp>

namespace game
{
    void demo_screen::draw(grfx::system& gfx, core::game_data& data) const
    {
        gfx.draw_patch(0, 0, *core::cache_lump<grfx::patch_t>(data, title_));
    }

}