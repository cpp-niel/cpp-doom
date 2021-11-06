#include <menu/draw_text.hpp>

#include <grfx/system.hpp>
#include <stdx/fold.hpp>

namespace menu
{
    namespace
    {
        constexpr auto default_space_width = 4;
        constexpr auto default_line_height = 12;

        int draw_char(grfx::system& gfx, const int x, const int y, const char ch)
        {
            if (const auto c = core::index_in_hud_font(ch); core::is_in_hud_font(c))
            {
                const auto& patch = gfx.hud_font_patch(c);
                gfx.draw_patch(x, y, patch);
                return patch.width;
            }

            return default_space_width;
        }
    }

    void draw_text(grfx::system& gfx, const int x, const int y, const std::string_view text)
    {
        stdx::fold(text | std::views::split('\n'), y, [&](const int cy, const auto& chars) {
            stdx::fold(chars, x, [&](const int cx, const char c) {
                return cx + draw_char(gfx, cx, cy, c);
            });

            return cy + default_line_height;
        });
    }
}