#pragma once

#include <core/game_data.hpp>
#include <grfx/grfx.hpp>
#include <grfx/screen_size.hpp>
#include <rndr/frame.hpp>
#include <rndr/lighting_tables.hpp>
#include <rndr/view.hpp>

#include <array>
#include <memory>
#include <span>

namespace rndr
{
    struct context_t;

    struct visplane_t
    {
        core::units height;
        int pic_num = 0;
        int light_level = 0;
        int min_x = 0;
        int max_x = 0;

        std::array<unsigned int, grfx::max_screen_width + 2> top{0xffffffffU};
        std::array<unsigned int, grfx::max_screen_width + 2> bottom{};
    };

    class visplanes
    {
    public:
        visplanes();
        ~visplanes();

        visplanes(const visplanes&) = delete;
        visplanes(visplanes&&) = delete;
        visplanes& operator=(const visplanes&) = delete;
        visplanes& operator=(visplanes&&) = delete;

        void clear();

        void calculate_y_slope(const int w, const int h);

        void map(const frame_t& frame, const view_t& view, const std::span<const std::uint8_t> source,
                 const std::optional<std::span<const light_table_t>>& fixed_color_map, const unsigned int y, const int x1,
                 const int x2);

        void make_spans(const frame_t& frame, const view_t& view, const std::span<const std::uint8_t> source,
                        const std::optional<std::span<const light_table_t>>& fixed_color_map, const int x,
                        unsigned int t1, unsigned int b1, unsigned int t2, unsigned int b2);

        void draw(const context_t& context);

        size_t find_plane_index(const int sky_flat_num, core::units height, const int pic_num, int light_level);

        size_t check_plane_index(const size_t index, const int start, const int stop);

        void set_extents(const size_t index, const int x, const int bottom, const int top);

    private:
        void draw_regular_plane(const context_t& context, visplane_t& pl);

        struct impl;
        std::unique_ptr<impl> impl_;
    };
}