#pragma once

#include <array>
#include <cstdint>

namespace grfx
{
    constexpr size_t icon_width = 128;
    constexpr size_t icon_height = 128;

    std::array<std::uint32_t, icon_width * icon_height> icon_data();
}