#pragma once

#include <string_view>

namespace grfx
{
    class system;
}

namespace menu
{
    void draw_text(grfx::system& gfx, const int x, const int y, const std::string_view text);
}