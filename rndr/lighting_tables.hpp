#pragma once

#include <core/real.hpp>
#include <grfx/grfx.hpp>

#include <array>
#include <span>

namespace rndr
{
    using light_table_t = grfx::pixel_t;

    constexpr auto num_light_scales = 48;
    constexpr auto max_light_scale = num_light_scales - 1;
    constexpr auto light_scale_factor = real{16};
    constexpr auto light_z_factor = real{1.0 / 16.0};
    constexpr auto light_seg_shift = 4;
    constexpr auto max_light_z = 128;
    constexpr auto light_levels = 16;
    constexpr auto light_bright = 1;

    struct lighting_tables_t
    {
        std::array<std::array<std::span<const light_table_t>, num_light_scales>, light_levels> scale_light;
        std::array<std::array<std::span<const light_table_t>, max_light_z>, light_levels> z_light;
        std::array<std::span<const light_table_t>, num_light_scales> scale_light_fixed;
    };
}