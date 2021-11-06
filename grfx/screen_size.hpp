#pragma once

namespace grfx
{
    constexpr auto original_screen_width = 320;
    constexpr auto original_screen_height = 200;
    constexpr auto screen_height = original_screen_height;
    constexpr auto aspect_corrected_screen_height = (6 * screen_height) / 5;
    constexpr auto standard_screen_width = original_screen_width;
    constexpr auto max_screen_width = original_screen_width * 4;
    constexpr auto max_screen_height = original_screen_height * 2;

    // todo !!
    constexpr auto window_width = 640;
    constexpr auto window_height = 400;

    struct adjusted_screen_size
    {
        int width = original_screen_width;
        int delta_width = 0;
    };
}