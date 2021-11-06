#pragma once

namespace grfx
{
    struct gfx_settings
    {
        bool is_fullscreen = false;
        int fullscreen_width = 0;
        int fullscreen_height = 0;
        bool is_software_renderer = false;
    };
}