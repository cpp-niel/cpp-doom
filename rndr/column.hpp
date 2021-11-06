#pragma once

#include <core/units.hpp>
#include <rndr/lighting_tables.hpp>

#include <cstdint>
#include <span>

namespace rndr
{
    struct context_t;
    struct view_t;

    struct draw_column_t
    {
        int x = 0;
        int y_start = 0;
        int y_end = 0;
        std::span<const light_table_t> color_map;
        real fraction_step{};
        real texture_mid{};
        std::span<const std::uint8_t> source;
    };

    std::span<const std::uint8_t> get_column(const context_t& context, const int tex, const unsigned int column);
    void draw_column(const view_t& view, const draw_column_t& dc);
}