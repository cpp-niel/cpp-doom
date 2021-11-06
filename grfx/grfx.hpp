#pragma once

#include <cstdint>
#include <span>

namespace grfx
{
    constexpr auto palette_size = 256;

    using pixel_t = std::uint8_t;

    extern std::span<pixel_t> video_buffer;  // todo - it's got to go
}