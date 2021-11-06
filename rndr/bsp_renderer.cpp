#include <rndr/bsp_renderer.hpp>

#include <core/math.hpp>
#include <core/wad_types.hpp>
#include <game/level.hpp>
#include <rndr/column.hpp>
#include <rndr/texture_info.hpp>
#include <rndr/trigonometry.hpp>
#include <rndr/visplane.hpp>
#include <stdx/on_exit.hpp>

#include <array>
#include <span>

namespace rndr
{
    namespace
    {
        using namespace ::core::literals;

        constexpr auto max_wall_scale = real{64};
        constexpr auto max_num_openings = grfx::max_screen_width * 64 * 4;
        constexpr auto texture_factor = real{16};

        struct visplane_indices_t
        {
            size_t floor_index = 0;
            size_t ceiling_index = 0;
        };

        struct regular_wall_t
        {
            int x = 0;
            int stop_x = 0;
            core::radians center_angle;
            core::units offset;
            core::units distance;
            real scale{};
            real scale_step{};
            real mid_texture_mid{};
            real top_texture_mid{};
            real bottom_texture_mid{};
            const std::array<std::span<const light_table_t>, num_light_scales>* lights = nullptr;
        };

        struct draw_texture_t
        {
            bool is_seg_textured = false;

            bool is_floor = false;
            bool is_ceiling = false;

            bool is_masked_texture = false;

            int top_texture = 0;
            int bottom_texture = 0;
            int mid_texture = 0;
            core::units world_top;
            core::units world_bottom;
            core::units world_high;
            core::units world_low;

            core::units pix_high;
            core::units pix_low;

            core::units pix_high_step;
            core::units pix_low_step;

            core::units top_fraction;
            core::units top_step;

            core::units bottom_fraction;
            core::units bottom_step;

            visplane_indices_t plane_indices;
        };

        auto to_tex_coord(const auto& x) { return x / texture_factor; }

        auto from_tex_coord(const auto& x) { return x * texture_factor; }

        real scale_from_global_angle(const context_t& context, const core::units wall_distance,
                                     const core::radians wall_normal_angle, core::radians vis_angle)
        {
            const auto angle_a = core::half_pi + vis_angle - context.frame.angle;
            const auto angle_b = core::half_pi + vis_angle - wall_normal_angle;
            const auto den = wall_distance * sin(angle_a);
            const auto num = core::units(static_cast<real>(context.view.projection)) * sin(angle_b);

            if (abs(den) < 1e-8_u) return max_wall_scale;

            return std::clamp(num / den, real(0.0039), max_wall_scale);
        }

        std::optional<std::pair<core::radians, core::radians>> clipped_angle_interval(const core::pos p1,
                                                                                      const core::pos p2,
                                                                                      const frame_t& frame,
                                                                                      const core::radians clip_angle)
        {
            auto a1 = normalize(point_to_angle(frame, p1) - frame.angle);
            auto a2 = normalize(point_to_angle(frame, p2) - frame.angle);

            const auto interval = normalize(a1 - a2);

            // Sitting on a line?
            if (interval >= core::pi) return std::nullopt;

            if (const auto a1_rot = normalize(a1 + clip_angle); a1_rot > 2.0 * clip_angle)
            {
                // Totally off the left edge?
                if (normalize(a1_rot - 2.0 * clip_angle) >= interval) return {};

                a1 = clip_angle;
            }

            if (!((a2 > (core::two_pi - clip_angle)) || (a2 < clip_angle))) a2 = core::two_pi - clip_angle;

            return std::pair(a1, a2);
        }

        bool check_bounding_box(const int view_width, const core::radians clip_angle, const frame_t& frame,
                                const clip_range_array& solid_segs, const game::bounding_box_t& box)
        {
            // Find the corners of the box
            // that define the edges from current viewpoint.
            const auto box_x = (frame.position.x <= box.left) ? 0 : ((frame.position.x < box.right) ? 1 : 2);
            const auto box_y = (frame.position.y >= box.top) ? 0 : ((frame.position.y > box.bottom) ? 1 : 2);
            const auto box_pos = (box_y << 2) + box_x;
            if (box_pos == 5) return true;

            using bb = game::bounding_box_t;
            constexpr auto check_coord = std::array{
                std::array{&bb::right, &bb::top, &bb::left, &bb::bottom},
                std::array{&bb::right, &bb::top, &bb::left, &bb::top},
                std::array{&bb::right, &bb::bottom, &bb::left, &bb::top},
                std::array{&bb::top, &bb::top, &bb::top, &bb::top},
                std::array{&bb::left, &bb::top, &bb::left, &bb::bottom},
                std::array{&bb::top, &bb::top, &bb::top, &bb::top},
                std::array{&bb::right, &bb::bottom, &bb::right, &bb::top},
                std::array{&bb::top, &bb::top, &bb::top, &bb::top},
                std::array{&bb::left, &bb::top, &bb::right, &bb::bottom},
                std::array{&bb::left, &bb::bottom, &bb::right, &bb::bottom},
                std::array{&bb::left, &bb::bottom, &bb::right, &bb::top},
                std::array{&bb::top, &bb::top, &bb::top, &bb::top},
            };

            const auto x1 = box.*check_coord[box_pos][0];
            const auto y1 = box.*check_coord[box_pos][1];
            const auto x2 = box.*check_coord[box_pos][2];
            const auto y2 = box.*check_coord[box_pos][3];

            const auto angle_range = clipped_angle_interval({x1, y1}, {x2, y2}, frame, clip_angle);
            if (!angle_range) return false;

            const auto [angle1, angle2] = *angle_range;
            if (normalize(angle1 - angle2) >= core::pi) return true;

            // Find the first clip post
            //  that touches the source post
            //  (adjacent pixels are touching).
            const auto sx1 = view_angle_to_x(view_width, clip_angle, angle1);
            const auto sx2 = view_angle_to_x(view_width, clip_angle, angle2) - 1;

            // Does not cross a pixel.
            if (sx1 == (sx2 + 1)) return false;

            const auto cr = *solid_segs.first_touching(sx2);
            return !(sx1 >= cr.first && sx2 <= cr.last);
        }

        int texture_translation(int x) { return x; }

        std::pair<int, int> mark_floors_and_ceilings(const int x, const core::units bottom_fraction,
                                                     const core::units top_fraction, const std::span<int> floor_clip,
                                                     const std::span<int> ceiling_clip, const draw_texture_t& dt,
                                                     visplanes& planes)
        {
            const auto add_plane = [&](const int unclipped_bottom, const int unclipped_top, const size_t plane_index) {
                const auto bottom = std::min(unclipped_bottom, floor_clip[x]) - 1;
                const auto top = std::max(unclipped_top, ceiling_clip[x]) + 1;
                if (top <= bottom) planes.set_extents(plane_index, x, bottom, top);
            };

            const auto wall_top = std::max(static_cast<int>(from_tex_coord(top_fraction)), ceiling_clip[x]) + 1;
            if (dt.is_ceiling) add_plane(wall_top, ceiling_clip[x], dt.plane_indices.ceiling_index);

            const auto wall_bottom = std::min(static_cast<int>(from_tex_coord(bottom_fraction)), floor_clip[x] - 1);
            if (dt.is_floor) add_plane(floor_clip[x], wall_bottom, dt.plane_indices.floor_index);

            return {wall_top, wall_bottom};
        }

        void draw_col(const context_t& context, const int start, const int end, const real texture_mid,
                      const int texture_index, const int texture_column, draw_column_t& dc)
        {
            dc.y_start = start;
            dc.y_end = end;
            dc.texture_mid = texture_mid;
            dc.source = get_column(context, texture_index, texture_column);
            draw_column(context.view, dc);
        }

        void draw_two_sided_column_piece(const context_t& context, const int texture_index, const int texture_column,
                                         const real texture_mid, const bool clip_at_bottom, const int plane_y,
                                         const bool has_plane, int& clip_value, draw_column_t& dc, const auto get_mid_y)
        {
            if (texture_index)
            {
                const auto mid_y = get_mid_y();
                const auto start = clip_at_bottom ? plane_y + 1 : mid_y;
                const auto end = clip_at_bottom ? mid_y : plane_y - 1;
                if (start <= end)
                {
                    draw_col(context, start, end, texture_mid, texture_index, texture_column, dc);
                    clip_value = clip_at_bottom ? end : start;
                }
                else
                {
                    clip_value = plane_y;
                }
            }
            else
            {
                if (has_plane) clip_value = plane_y;
            }
        }

        void set_wall_texture_coordinates(const context_t& context, const game::sector_t& front_sector,
                                          const game::sector_t* back_sector, const game::seg_t& line,
                                          regular_wall_t& wall)
        {
            const auto front_ceiling_height = front_sector.ceiling_height - context.frame.z;
            const auto front_floor_height = front_sector.floor_height - context.frame.z;
            if (back_sector != nullptr)
            {
                const auto back_ceiling_height = back_sector->ceiling_height - context.frame.z;
                if (back_ceiling_height < front_ceiling_height)
                {
                    const auto is_pegged = (line.line_def->flags & core::line_def_flags::dont_peg_top) == 0;
                    wall.top_texture_mid = static_cast<real>(
                        line.side_def->row_offset
                        + (is_pegged ? (back_ceiling_height + context.texture_info.height[line.side_def->top_texture])
                                     : front_ceiling_height));
                }

                const auto back_floor_height = back_sector->floor_height - context.frame.z;
                if (back_floor_height > front_floor_height)
                {
                    const auto is_pegged = (line.line_def->flags & core::line_def_flags::dont_peg_bottom) == 0;
                    wall.bottom_texture_mid = static_cast<real>(
                        line.side_def->row_offset + (is_pegged ? back_floor_height : front_ceiling_height));
                }
            }
            else
            {
                const auto is_pegged = (line.line_def->flags & core::line_def_flags::dont_peg_bottom) == 0;
                wall.mid_texture_mid = static_cast<real>(
                    line.side_def->row_offset
                    + (is_pegged ? front_ceiling_height
                                 : front_floor_height + context.texture_info.height[line.side_def->mid_texture]));
            }
        }

        bool is_closed_door(const game::sector_t& front_sector, const game::sector_t* back_sector)
        {
            return (back_sector != nullptr)
                   && (back_sector->ceiling_height <= front_sector.floor_height
                       || back_sector->floor_height >= front_sector.ceiling_height);
        }

        draw_texture_t texture_drawing_information(const context_t& context, const game::side_t& side,
                                                   const game::sector_t& front_sector,
                                                   const game::sector_t* back_sector)
        {
            const auto is_sector_outdoors = [&](const game::sector_t& sector) {
                return sector.ceiling_pic == context.level.sky_flat_num;
            };

            draw_texture_t dt;
            dt.world_top = to_tex_coord(front_sector.ceiling_height - context.frame.z);
            dt.world_bottom = to_tex_coord(front_sector.floor_height - context.frame.z);
            if (back_sector == nullptr)
            {
                // single sided line
                dt.mid_texture = texture_translation(side.mid_texture);
                dt.is_floor = true;
                dt.is_ceiling = true;
            }
            else
            {
                // two-sided line
                dt.world_high = to_tex_coord(back_sector->ceiling_height - context.frame.z);
                dt.world_low = to_tex_coord(back_sector->floor_height - context.frame.z);

                // hack to allow height changes in outdoor areas
                if (is_sector_outdoors(front_sector) && is_sector_outdoors(*back_sector)) dt.world_top = dt.world_high;

                dt.is_floor = (dt.world_low != dt.world_bottom || back_sector->floor_pic != front_sector.floor_pic
                               || back_sector->light_level != front_sector.light_level);

                dt.is_ceiling = (dt.world_high != dt.world_top || back_sector->ceiling_pic != front_sector.ceiling_pic
                                 || back_sector->light_level != front_sector.light_level);

                if (is_closed_door(front_sector, back_sector))
                {
                    dt.is_ceiling = true;
                    dt.is_floor = true;
                }

                if (dt.world_high < dt.world_top) dt.top_texture = texture_translation(side.top_texture);

                if (dt.world_low > dt.world_bottom) dt.bottom_texture = texture_translation(side.bottom_texture);

                if ((side.mid_texture) != 0) dt.is_masked_texture = true;
            }

            dt.is_seg_textured =
                (dt.mid_texture != 0) || (dt.top_texture != 0) || (dt.bottom_texture != 0) || dt.is_masked_texture;

            // if a floor / ceiling plane is on the wrong side
            //  of the view plane, it is definitely invisible
            //  and doesn't need to be marked.
            if (front_sector.floor_height >= context.frame.z) dt.is_floor = false;

            if ((front_sector.ceiling_height <= context.frame.z) && !is_sector_outdoors(front_sector))
                dt.is_ceiling = false;

            return dt;
        }

        int light_table_index(const context_t& context, const game::sector_t& sector, const core::pos& v1,
                              const core::pos& v2)
        {
            const auto x_offset = (v1.x == v2.x) ? 1 : 0;
            const auto y_offset = (v1.y == v2.y) ? -1 : 0;
            const auto light_num = (sector.light_level >> light_seg_shift) + context.frame.extra_light;
            return std::clamp(light_num + x_offset + y_offset, 0, light_levels - 1);
        }

        draw_seg_t segment_drawing_information(const context_t& context, const game::seg_t& line, const int first,
                                               const int last, const core::radians wall_normal_angle,
                                               const core::units wall_distance)
        {
            draw_seg_t ds{.line = &line, .x1 = first, .x2 = last};

            // calculate scale at both ends and step
            const auto first_angle = x_to_view_angle(context.view.width, context.view.clip_angle, first);
            ds.scale1 =
                scale_from_global_angle(context, wall_distance, wall_normal_angle, context.frame.angle + first_angle);

            if (last > first)
            {
                const auto last_angle = x_to_view_angle(context.view.width, context.view.clip_angle, last);
                ds.scale2 =
                    scale_from_global_angle(context, wall_distance, wall_normal_angle, context.frame.angle + last_angle);
                ds.scale_step = (ds.scale2 - ds.scale1) / static_cast<real>(last - first);
            }
            else
            {
                ds.scale2 = ds.scale1;
            }

            ds.masked_texture_col = nullptr;

            // todo R_StoreWallRange silhouette stuff
            // if (back_sector == nullptr)
            //    ...
            // else
            //    ...

            return ds;
        }
    }

    class bsp_renderer::impl
    {
    public:
        void render_seg_loop(const context_t& context, const draw_texture_t& dt, const regular_wall_t& wall);

        void store_wall_range(const context_t& context, const visplane_indices_t& plane_indices,
                              const game::sector_t& front_sector, const game::sector_t* back_sector,
                              const game::seg_t& line, const int first, const int last);

        template <bool IsSolid>
        void clip_wall_segment(const context_t& context, const visplane_indices_t& plane_indices,
                               const game::sector_t& front_sector, const game::sector_t* back_sector,
                               const game::seg_t& line, const int first, const int last);

        void render_line(const context_t& context, const visplane_indices_t& plane_indices,
                         const game::sector_t& front_sector, const game::seg_t& line);

        void render_sub_sector(const context_t& context, const int sub_sector_num);

        void render_bsp_node(const context_t& context, const int node_num);

        void reset(const int view_width, const int view_height);

        void draw_visplanes(const context_t& context) { visplanes_.draw(context); }

        void on_view_size_changed(const int view_width, const int view_height)
        {
            visplanes_.calculate_y_slope(view_width, view_height);
        }

    private:
        clip_range_array solid_segs;
        std::vector<draw_seg_t> draw_segs;
        std::array<int, grfx::max_screen_width> floor_clip{};
        std::array<int, grfx::max_screen_width> ceiling_clip{};

        std::array<short, max_num_openings> openings{};
        int last_opening_index = 0;

        visplanes visplanes_;
    };

    void bsp_renderer::impl::render_seg_loop(const context_t& context, const draw_texture_t& dt,
                                             const regular_wall_t& wall)
    {
        int texture_column = 0;
        auto pix_high = dt.pix_high;
        auto pix_low = dt.pix_low;
        auto bottom_fraction = dt.bottom_fraction;
        auto top_fraction = dt.top_fraction;
        auto wall_scale = wall.scale;
        for (auto x = wall.x; x < wall.stop_x; ++x)
        {
            // mark floor / ceiling areas
            const auto [yl, yh] =
                mark_floors_and_ceilings(x, bottom_fraction, top_fraction, floor_clip, ceiling_clip, dt, visplanes_);

            draw_column_t dc;
            // texture column and lighting are independent of wall tiers
            if (dt.is_seg_textured)
            {
                // calculate texture offset
                const auto view_angle = x_to_view_angle(context.view.width, context.view.clip_angle, x);
                const auto angle = wall.center_angle + view_angle - (0.5 * core::pi);
                texture_column = static_cast<int>(wall.offset - (tan(angle) * wall.distance));
                // calculate lighting
                const auto index = std::min(static_cast<int>(wall_scale * light_scale_factor), max_light_scale);

                dc.color_map = (*wall.lights)[index];
                dc.x = x;
                dc.fraction_step = real{1} / wall_scale;
            }

            // draw the wall tiers
            if (dt.mid_texture != 0)
            {
                // single sided line
                draw_col(context, yl, yh, wall.mid_texture_mid, dt.mid_texture, texture_column, dc);
                ceiling_clip[x] = context.view.height;
                floor_clip[x] = -1;
            }
            else
            {
                // two-sided line - draw the top piece...
                draw_two_sided_column_piece(
                    context, dt.top_texture, texture_column, wall.top_texture_mid, true, yl - 1, dt.is_ceiling,
                    ceiling_clip[x], dc, [&] {
                        return std::min(std::min(context.view.height - 1, static_cast<int>(from_tex_coord(pix_high))),
                                        floor_clip[x] - 1);
                    });
                if (dt.top_texture != 0) pix_high += dt.pix_high_step;

                // ...then the bottom piece
                draw_two_sided_column_piece(context, dt.bottom_texture, texture_column, wall.bottom_texture_mid, false,
                                            yh + 1, dt.is_floor, floor_clip[x], dc, [&] {
                                                return std::max(
                                                    std::max(0, static_cast<int>(from_tex_coord(pix_low)) + 1),
                                                    ceiling_clip[x] + 1);
                                            });
                if (dt.bottom_texture != 0) pix_low += dt.pix_low_step;

                if (dt.is_masked_texture)
                {
                    // save texture_col
                    //  for back drawing of masked mid texture
                    // todo
                    // masked_texture_col[x] = texture_column;
                }
            }

            wall_scale += wall.scale_step;
            bottom_fraction += dt.bottom_step;
            top_fraction += dt.top_step;
        }
    }

    void bsp_renderer::impl::store_wall_range(const context_t& context, const visplane_indices_t& plane_indices,
                                              const game::sector_t& front_sector, const game::sector_t* back_sector,
                                              const game::seg_t& line, const int first, const int last)
    {
        // mark the segment as visible for auto map
        // line_def->flags |= core::line_def_flags::mapped;  // todo
        const auto wall_normal_angle = line.angle + core::half_pi;

        const auto wall_distance = core::distance_from_point_to_line(context.frame.position, *line.v1, *line.v2);

        auto ds = segment_drawing_information(context, line, first, last, wall_normal_angle, wall_distance);

        auto dt = texture_drawing_information(context, *line.side_def, front_sector, back_sector);

        // allocate space for masked texture tables
        if (dt.is_masked_texture)
        {
            ds.masked_texture_col = &openings[last_opening_index - first];
            last_opening_index += last - first + 1;
        }

        regular_wall_t wall{.x = first, .stop_x = last + 1, .distance = wall_distance};
        wall.scale = static_cast<real>(ds.scale1);
        if (last > first) wall.scale_step = static_cast<real>(ds.scale_step);

        if (dt.is_seg_textured)
        {
            set_wall_texture_coordinates(context, front_sector, back_sector, line, wall);
            wall.offset = core::units{core::dot(context.frame.position - *line.v1, *line.v2 - *line.v1) / line.length};
            wall.offset += line.side_def->texture_offset + line.offset;
            wall.center_angle = core::half_pi + context.frame.angle - wall_normal_angle;
            wall.lights =
                context.fixed_color_map
                    ? &context.lighting_tables.scale_light_fixed
                    : &context.lighting_tables.scale_light[light_table_index(context, front_sector, *line.v1, *line.v2)];
        }

        const auto center_y_fraction = to_tex_coord(context.view.center_y_fraction);

        // calculate incremental stepping values for texture edges
        dt.top_step = -wall.scale_step * dt.world_top;
        dt.top_fraction = center_y_fraction - (dt.world_top * wall.scale);

        dt.bottom_step = -wall.scale_step * dt.world_bottom;
        dt.bottom_fraction = center_y_fraction - (dt.world_bottom * wall.scale);

        if (back_sector != nullptr)
        {
            if (dt.world_high < dt.world_top)
            {
                dt.pix_high = center_y_fraction - (dt.world_high * wall.scale);
                dt.pix_high_step = -(wall.scale_step * dt.world_high);
            }

            if (dt.world_low > dt.world_bottom)
            {
                dt.pix_low = center_y_fraction - (dt.world_low * wall.scale);
                dt.pix_low_step = -(wall.scale_step * dt.world_low);
            }
        }

        // render it
        if (dt.is_ceiling)
        {
            dt.plane_indices.ceiling_index =
                visplanes_.check_plane_index(plane_indices.ceiling_index, wall.x, wall.stop_x - 1);
        }

        if (dt.is_floor)
        {
            dt.plane_indices.floor_index =
                visplanes_.check_plane_index(plane_indices.floor_index, wall.x, wall.stop_x - 1);
        }

        render_seg_loop(context, dt, wall);

        // todo - save sprite clipping info

        draw_segs.push_back(ds);
    }

    template <bool IsSolid>
    void bsp_renderer::impl::clip_wall_segment(const context_t& context, const visplane_indices_t& plane_indices,
                                               const game::sector_t& front_sector, const game::sector_t* back_sector,
                                               const game::seg_t& line, const int first, const int last)
    {
        // Find the first range that touches the range
        //  (adjacent pixels are touching).
        auto* start = solid_segs.first_touching(first - 1);

        if (first < start->first)
        {
            if (last < start->first - 1)
            {
                // Post is entirely visible (above start),
                //  so insert a new clip post.
                store_wall_range(context, plane_indices, front_sector, back_sector, line, first, last);
                if constexpr (IsSolid) solid_segs.insert(start, first, last);

                return;
            }

            // There is a fragment above *start.
            store_wall_range(context, plane_indices, front_sector, back_sector, line, first, start->first - 1);
        }

        if constexpr (IsSolid)
        {
            // Now adjust the clip size.
            if (first < start->first) start->first = first;
        }

        // Bottom contained in start?
        if (last <= start->last) return;

        auto* current = start;

        const auto crunch = stdx::on_exit(std::function([&] {
            if constexpr (IsSolid)
                if (current != start) solid_segs.remove(std::next(start), current);
        }));

        while (last >= std::next(current)->first - 1)
        {
            // There is a fragment between two posts.
            store_wall_range(context, plane_indices, front_sector, back_sector, line, current->last + 1,
                             std::next(current)->first - 1);
            current = std::next(current);

            if (last <= current->last)
            {
                // Bottom is contained in next.
                // Adjust the clip size.
                if constexpr (IsSolid) start->last = current->last;
                return;
            }
        }

        // There is a fragment after *next.
        store_wall_range(context, plane_indices, front_sector, back_sector, line, current->last + 1, last);
        // Adjust the clip size.
        if constexpr (IsSolid) start->last = last;
    }

    void bsp_renderer::impl::render_line(const context_t& context, const visplane_indices_t& plane_indices,
                                         const game::sector_t& front_sector, const game::seg_t& line)
    {
        const auto angle_range = clipped_angle_interval(*line.v1, *line.v2, context.frame, context.view.clip_angle);
        if (!angle_range) return;

        const auto [angle1, angle2] = *angle_range;
        if (normalize(angle1 - angle2) >= core::pi) return;

        // The seg is in the view range,
        // but not necessarily visible.
        const auto x1 = view_angle_to_x(context.view.width, context.view.clip_angle, angle1);
        const auto x2 = view_angle_to_x(context.view.width, context.view.clip_angle, angle2);

        // Does not cross a pixel?
        if (x1 == x2) return;

        const auto* back_sector = line.back_sector;

        // Single sided line?
        if ((back_sector == nullptr) || is_closed_door(front_sector, back_sector))
        { clip_wall_segment<true>(context, plane_indices, front_sector, back_sector, line, x1, x2 - 1); }
        else
        {
            const bool is_window = ((back_sector->ceiling_height != front_sector.ceiling_height
                                     || back_sector->floor_height != front_sector.floor_height));

            // Reject empty lines used for triggers
            //  and special events.
            // Identical floor and ceiling on both sides,
            // identical light levels on both sides,
            // and no middle texture.
            const bool is_rejected =
                ((back_sector->ceiling_pic == front_sector.ceiling_pic)
                 && (back_sector->floor_pic == front_sector.floor_pic)
                 && (back_sector->light_level == front_sector.light_level) && (line.side_def->mid_texture == 0));

            if (is_window || !is_rejected)
                clip_wall_segment<false>(context, plane_indices, front_sector, back_sector, line, x1, x2 - 1);
        }
    }

    void bsp_renderer::impl::render_sub_sector(const context_t& context, const int sub_sector_num)
    {
        const auto& sub = context.level.sub_sectors[sub_sector_num];
        const auto* front_sector = sub.sector;

        visplane_indices_t plane_indices;
        if (front_sector->floor_height < context.frame.z)
        {
            plane_indices.floor_index =
                visplanes_.find_plane_index(context.level.sky_flat_num, front_sector->floor_height,
                                            front_sector->floor_pic, front_sector->light_level);
        }

        if (front_sector->ceiling_height > context.frame.z || front_sector->ceiling_pic == context.level.sky_flat_num)
        {
            plane_indices.ceiling_index =
                visplanes_.find_plane_index(context.level.sky_flat_num, front_sector->ceiling_height,
                                            front_sector->ceiling_pic, front_sector->light_level);
        }

        const auto first_seg = std::next(context.level.segs.begin(), sub.first_line);
        for (const auto& seg : std::span(first_seg, sub.num_lines))
            render_line(context, plane_indices, *front_sector, seg);
    }

    void bsp_renderer::impl::render_bsp_node(const context_t& context, const int node_num)
    {
        const auto num = static_cast<unsigned short>(node_num);
        if ((num & core::node_flags::sub_sector) != 0)
        {
            const auto sub_sector_num = (num == core::node_flags::none) ? 0 : (num & (~core::node_flags::sub_sector));
            render_sub_sector(context, sub_sector_num);
            return;
        }

        const auto& bsp_node = context.level.nodes[node_num];

        const auto side = core::point_on_side(context.frame.position, bsp_node);

        render_bsp_node(context, bsp_node.children[static_cast<int>(side)].index);

        const auto other_side_child = bsp_node.children[static_cast<int>(side) ^ 1];
        if (check_bounding_box(context.view.width, context.view.clip_angle, context.frame, solid_segs,
                               other_side_child.bbox))
            render_bsp_node(context, other_side_child.index);
    }

    void bsp_renderer::impl::reset(const int view_width, const int view_height)
    {
        std::ranges::fill(floor_clip, view_height);
        std::ranges::fill(ceiling_clip, -1);

        solid_segs.reset(view_width);
        draw_segs.clear();

        visplanes_.clear();
        last_opening_index = 0;
    }

    bsp_renderer::bsp_renderer() : impl_(std::make_unique<impl>()) {}

    bsp_renderer::~bsp_renderer() = default;

    void bsp_renderer::render_bsp_node(const context_t& context, const int node_num)
    {
        impl_->reset(context.view.width, context.view.height);
        impl_->render_bsp_node(context, node_num);
        impl_->draw_visplanes(context);
    }

    void bsp_renderer::on_view_size_changed(const int view_width, const int view_height)
    {
        impl_->on_view_size_changed(view_width, view_height);
    }
}