#include <rndr/visplane.hpp>

#include <game/level.hpp>
#include <rndr/column.hpp>
#include <rndr/context.hpp>
#include <rndr/sky.hpp>
#include <rndr/texture_info.hpp>
#include <rndr/trigonometry.hpp>

#include <vector>

namespace rndr
{
    namespace
    {
        using namespace ::core::literals;

        constexpr auto plane_sky_flat = 0x80000000;

        struct span_t
        {
            unsigned int y = 0;
            int x_start = 0;
            int x_end = 0;
            core::units u_start;
            core::units v_start;
            core::units u_step;
            core::units v_step;
            std::span<const std::uint8_t> source;
            std::span<const light_table_t> color_map;
            int view_width = 0;

            // std::uint8_t* bright_map;
        };

        auto source_pixel(const core::units u, const core::units v, const std::span<const std::uint8_t> source)
        {
            // const auto index = ((v.raw_value() >> (16 - 6)) & (63 * 64)) + (static_cast<int>(u) & 63);
            const auto index = (static_cast<int>(v * 64) & (63 * 64)) + (static_cast<int>(u) & 63);
            return source[index];
        }

        void draw_span(const span_t& ds)
        {
            const auto span_size = ds.x_end - ds.x_start;
            const auto dest_pixels = grfx::video_buffer.subspan(ds.y * ds.view_width + ds.x_start, span_size);
            auto u = ds.u_start;
            auto v = ds.v_start;
            for (auto& dest_pixel : dest_pixels)
            {
                dest_pixel = ds.color_map[source_pixel(u, v, ds.source)];

                u += ds.u_step;
                v += ds.v_step;
            }
        }

        void draw_sky_plane(const context_t& context, const visplane_t& pl)
        {
            // Sky is always drawn full bright,
            //  i.e. color_maps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
            // todo this should be computed in rndr::system when the view size changes
            const auto pspriteiscale = real{1};
            auto dc = draw_column_t{
                .color_map = context.color_maps, .fraction_step = pspriteiscale, .texture_mid = sky_texture_mid};

            for (int x = pl.min_x; x <= pl.max_x; ++x)
            {
                const auto y_start = pl.top[x + 1];
                const auto y_end = pl.bottom[x + 1];

                if (y_start <= y_end)
                {
                    dc.y_start = static_cast<int>(y_start);
                    dc.y_end = static_cast<int>(y_end);
                    const auto view_angle = x_to_view_angle(context.view.width, context.view.clip_angle, x);
                    const auto col =
                        static_cast<unsigned int>(to_fraction(context.frame.angle + view_angle) * sky_column_factor);
                    dc.x = x;
                    dc.source = get_column(context, context.level.sky_texture, col);
                    draw_column(context.view, dc);
                }
            }
        }
    }

    struct visplanes::impl
    {
        std::vector<visplane_t> visplanes;
        // visplane_t* floor_plane = nullptr;
        // visplane_t* ceiling_plane = nullptr;
        // int num_visplanes = 0;

        //
        // span_start holds the start of a plane span
        // initialized to 0 at start
        //
        std::array<int, grfx::max_screen_height> span_start{};
        // std::array<int, grfx::max_screen_height> span_stop{};

        //
        // texture mapping
        //
        const std::array<std::span<const light_table_t>, max_light_z>* plane_z_light = nullptr;
        core::units plane_height;

        std::array<real, grfx::screen_height> y_slope{};
        // std::array<real, grfx::max_screen_width> dist_scale{};

        std::array<core::units, grfx::max_screen_height> cached_height{};
        std::array<core::units, grfx::max_screen_height> cached_distance{};
        std::array<core::units, grfx::max_screen_height> cached_x_step{};
        std::array<core::units, grfx::max_screen_height> cached_y_step{};
    };

    visplanes::visplanes() : impl_(std::make_unique<impl>()) {}

    visplanes::~visplanes() = default;

    void visplanes::calculate_y_slope(const int w, const int h)
    {
        constexpr auto half = real{0.5};
        const auto half_width = static_cast<real>(w) * half;
        const auto half_height = static_cast<real>(h) * half;
        for (int i = 0; i < h; i++)
        {
            const auto dy = std::abs(static_cast<real>(i) - half_height) + half;
            impl_->y_slope[i] = half_width / dy;
        }
    }

    void visplanes::clear()
    {
        impl_->visplanes.clear();
        std::ranges::fill(impl_->cached_height, 0_u);
    }

    void visplanes::map(const frame_t& frame, const view_t& view, const std::span<const std::uint8_t> source,
                        const std::optional<std::span<const light_table_t>>& fixed_color_map, const unsigned int y,
                        const int x1, const int x2)
    {
        const auto dy = static_cast<real>(abs(view.center_y - static_cast<int>(y)));
        if (dy == 0) return;

        const auto view_sin = sin(frame.angle);
        const auto view_cos = cos(frame.angle);

        if (impl_->plane_height != impl_->cached_height[y])
        {
            impl_->cached_height[y] = impl_->plane_height;
            impl_->cached_distance[y] = impl_->plane_height * impl_->y_slope[y];
            impl_->cached_x_step[y] = (view_sin * impl_->plane_height) / dy;
            impl_->cached_y_step[y] = (view_cos * impl_->plane_height) / dy;
        }

        const auto dx = static_cast<real>(x1 - view.center_x);
        const auto distance = impl_->cached_distance[y];
        const auto u_step = impl_->cached_x_step[y];
        const auto v_step = impl_->cached_y_step[y];

        draw_span({.y = y,
                   .x_start = x1,
                   .x_end = x2,
                   .u_start = frame.position.x + (view_cos * distance) + dx * u_step,
                   .v_start = -frame.position.y - (view_sin * distance) + dx * v_step,
                   .u_step = u_step,
                   .v_step = v_step,
                   .source = source,
                   .color_map = fixed_color_map ? *fixed_color_map
                                                : (*impl_->plane_z_light)[std::clamp(
                                                    static_cast<int>(distance * light_z_factor), 0, max_light_z - 1)],
                   .view_width = view.width});
    }

    void visplanes::make_spans(const frame_t& frame, const view_t& view, const std::span<const std::uint8_t> source,
                               const std::optional<std::span<const light_table_t>>& fixed_color_map, const int x,
                               unsigned int t1, unsigned int b1, unsigned int t2, unsigned int b2)
    {
        while (t1 < t2 && t1 <= b1)
        {
            map(frame, view, source, fixed_color_map, t1, impl_->span_start[t1], x);
            t1++;
        }
        while (b1 > b2 && b1 >= t1)
        {
            map(frame, view, source, fixed_color_map, b1, impl_->span_start[b1], x);
            b1--;
        }

        while (t2 < t1 && t2 <= b2)
        {
            impl_->span_start[t2] = x;
            t2++;
        }
        while (b2 > b1 && b2 >= t2)
        {
            impl_->span_start[b2] = x;
            b2--;
        }
    }

    void visplanes::draw(const context_t& context)
    {
        for (auto& pl : impl_->visplanes)
        {
            if (pl.min_x > pl.max_x) continue;

            if (pl.pic_num == context.level.sky_flat_num)
                draw_sky_plane(context, pl);
            else
                draw_regular_plane(context, pl);
        }
    }

    void visplanes::draw_regular_plane(const context_t& context, visplane_t& pl)
    {
        const auto source = core::cache_lump_num_as_span<uint8_t>(context.data, pl.pic_num);

        impl_->plane_height = abs(pl.height - context.frame.z);
        const auto light = std::clamp((pl.light_level >> light_seg_shift) + (context.frame.extra_light * light_bright),
                                      0, light_levels - 1);

        impl_->plane_z_light = &context.lighting_tables.z_light[light];

        pl.top[pl.max_x + 2] = 0xffffffffU;
        pl.top[pl.min_x] = 0xffffffffU;

        const auto stop = pl.max_x + 1;

        for (auto x = pl.min_x; x <= stop; x++)
        {
            make_spans(context.frame, context.view, source, context.fixed_color_map, x, pl.top[x], pl.bottom[x],
                       pl.top[x + 1], pl.bottom[x + 1]);
        }
    }

    size_t visplanes::find_plane_index(const int sky_flat_num, core::units height, const int pic_num, int light_level)
    {
        if ((pic_num == sky_flat_num) || ((pic_num & plane_sky_flat) != 0))
        {
            height = 0_u;  // all skies map together
            light_level = 0;
        }

        const auto it = std::ranges::find_if(impl_->visplanes, [&](const visplane_t& p) {
            return (p.height == height) && (p.pic_num == pic_num) && (p.light_level == light_level);
        });

        if (it != impl_->visplanes.end()) return std::distance(impl_->visplanes.begin(), it);

        impl_->visplanes.push_back({.height = height,
                                    .pic_num = pic_num,
                                    .light_level = light_level,
                                    .min_x = grfx::standard_screen_width,
                                    .max_x = -1});
        return impl_->visplanes.size() - 1;
    }

    size_t visplanes::check_plane_index(const size_t index, const int start, const int stop)
    {
        auto& pl = impl_->visplanes[index];

        const auto union_low = std::min(start, pl.min_x);
        const auto [intersect_high, union_high] = std::minmax(stop, pl.max_x);

        const auto* const it = std::ranges::find_if(pl.top, [](const auto t) { return t != 0xffffffffU; });

        if ((it != pl.top.end()) && (*it > intersect_high))
        {
            pl.min_x = union_low;
            pl.max_x = union_high;

            // use the same one
            return index;
        }

        impl_->visplanes.push_back(
            {.height = pl.height, .pic_num = pl.pic_num, .light_level = pl.light_level, .min_x = start, .max_x = stop});
        return impl_->visplanes.size() - 1;
    }

    void visplanes::set_extents(const size_t index, const int x, const int bottom, const int top)
    {
        impl_->visplanes[index].bottom[x + 1] = bottom;
        impl_->visplanes[index].top[x + 1] = top;
    }
}
