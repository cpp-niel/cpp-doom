#pragma once

#include <rndr/lighting_tables.hpp>

#include <optional>
#include <span>

namespace core
{
    struct game_data;
}

namespace game
{
    struct level_t;
}

namespace rndr
{
    struct frame_t;
    struct texture_info_t;
    struct view_t;

    struct context_t
    {
        core::game_data& data;
        const frame_t& frame;
        const game::level_t& level;
        texture_info_t& texture_info;
        const lighting_tables_t& lighting_tables;
        const view_t& view;
        std::span<const light_table_t> color_maps;
        std::optional<std::span<const light_table_t>> fixed_color_map;
    };

}