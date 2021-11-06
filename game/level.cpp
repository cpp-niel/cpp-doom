#include <game/level.hpp>

#include <core/game_data.hpp>
#include <core/wad_types.hpp>
#include <rndr/system.hpp>
#include <stdx/to.hpp>

namespace game
{
    namespace
    {
        template <typename MapType, typename Conversion>
        auto load(core::game_data& data, const size_t lump, const Conversion to_game_type)
        {
            const auto map_vertices = core::cache_lump_num_as_span<MapType>(data, lump);
            return map_vertices | std::views::transform(to_game_type) | stdx::to<std::vector>();
        }

        template <typename T>
        constexpr auto min_max(const T x, const T y)
        {
            return (x < y) ? std::pair(x, y) : std::pair(y, x);
        }

        constexpr bounding_box_t get_bounding_box(const core::pos& p0, const core::pos& p1)
        {
            bounding_box_t result;
            std::tie(result.left, result.right) = min_max(p0.x, p1.x);
            std::tie(result.bottom, result.top) = min_max(p0.y, p1.y);
            return result;
        }

        blockmap_t load_blockmap(core::game_data& data, const size_t lump)
        {
            struct blockmap_info
            {
                short x, y, width, height;
            };

            auto stream = core::lump_byte_stream(data, lump);
            const auto info = stream.read<blockmap_info>();
            const auto map = stream.read_span<short>(static_cast<size_t>(info.width * info.height));
            return {.width = info.width,
                    .height = info.height,
                    .map = map,
                    .x = core::units(info.x),
                    .y = core::units(info.y)};

            // todo clear out mobj chains
            // const auto count = sizeof(*blocklinks) * bmapwidth*bmapheight;
            // blocklinks = Z_Malloc (count,PU_LEVEL, 0);
            // memset (blocklinks, 0, count);
        }

        vertices_t load_vertices(core::game_data& data, const size_t lump)
        {
            return load<core::map_vertex>(data, lump, [](const core::map_vertex& v) {
                return core::pos{.x = core::units(v.x), .y = core::units(v.y)};
            });
        }

        sectors_t load_sectors(core::game_data& data, const size_t lump)
        {
            return load<core::map_sector>(data, lump, [&](const core::map_sector& s) {
                return sector_t{.floor_height = core::units(s.floor_height),
                                .ceiling_height = core::units(s.ceiling_height),
                                .floor_pic = static_cast<short>(data.lump_table[core::array_to_string(s.floor_pic)]),
                                .ceiling_pic =
                                    static_cast<short>(data.lump_table[core::array_to_string(s.ceiling_pic)]),
                                .light_level = s.light_level,
                                .special = s.special,
                                .tag = s.tag};
            });
        }

        sides_t load_sides(core::game_data& data, const rndr::system& renderer, sectors_t& sectors, const size_t lump)
        {
            return load<core::map_side_def>(data, lump, [&](const core::map_side_def& s) {
                return side_t{.texture_offset = core::units(s.texture_offset),
                              .row_offset = core::units(s.row_offset),
                              .top_texture = renderer.texture_num(core::array_to_string(s.top_texture)),
                              .bottom_texture = renderer.texture_num(core::array_to_string(s.bottom_texture)),
                              .mid_texture = renderer.texture_num(core::array_to_string(s.mid_texture)),
                              .sector = &sectors[s.sector]};
            });
        }

        slope_type_t get_slope_type(const core::units dx, const core::units dy)
        {
            return is_zero(dx)
                       ? slope_type_t::vertical
                       : (is_zero(dy) ? slope_type_t::horizontal
                                      : (((dy / dx) > real{0}) ? slope_type_t::positive : slope_type_t::negative));
        }

        lines_t load_lines(core::game_data& data, const vertices_t& vertices, const sides_t& sides, const size_t lump)
        {
            return load<core::map_line_def>(data, lump, [&](const core::map_line_def& m) {
                const auto* v1 = &vertices[m.v1];
                const auto* v2 = &vertices[m.v2];
                const auto dx = v2->x - v1->x;
                const auto dy = v2->y - v1->y;
                return line_t{.v1 = v1,
                              .v2 = v2,
                              .dx = dx,
                              .dy = dy,
                              .flags = m.flags,
                              .special = m.special,
                              .tag = m.tag,
                              .side_num = m.side_num,
                              .bbox = get_bounding_box(*v1, *v2),
                              .slope_type = get_slope_type(dx, dy),
                              .front_sector = (m.side_num[0] != -1) ? sides[m.side_num[0]].sector : nullptr,
                              .back_sector = (m.side_num[1] != -1) ? sides[m.side_num[1]].sector : nullptr};
            });
        }

        sub_sectors_t load_sub_sectors(core::game_data& data, const size_t lump)
        {
            return load<core::map_sub_sector>(data, lump, [&](const core::map_sub_sector& s) {
                return sub_sector_t{.num_lines = s.num_segs, .first_line = s.first_seg};
            });
        }

        nodes_t load_nodes(core::game_data& data, const size_t lump)
        {
            const auto get_child = [](const unsigned short index, const std::array<short, 4>& bbox) {
                return node_child_t{.index = index,
                                    .bbox = {.top = core::units(bbox[0]),
                                             .bottom = core::units(bbox[1]),
                                             .left = core::units(bbox[2]),
                                             .right = core::units(bbox[3])}};
            };

            return load<core::map_node_t>(data, lump, [&](const core::map_node_t& n) {
                return node_t{
                    .x = core::units(n.x),
                    .y = core::units(n.y),
                    .dx = static_cast<real>(n.dx),
                    .dy = static_cast<real>(n.dy),
                    .children = {get_child(n.children[0], n.bboxes[0]), get_child(n.children[1], n.bboxes[1])}};
            });
        }

        segs_t load_segs(core::game_data& data, const vertices_t& vertices, const lines_t& lines, const sides_t& sides,
                         const size_t lump)
        {
            return load<core::map_seg_t>(data, lump, [&](const core::map_seg_t& s) {
                const auto* line_def = &lines[s.line_def];
                const auto* v1 = &vertices[s.v1];
                const auto* v2 = &vertices[s.v2];
                return seg_t{.v1 = v1,
                             .v2 = v2,
                             .offset = core::units(s.offset),
                             .angle = core::radians::from_doom_angle(s.angle),
                             .side_def = &sides[line_def->side_num[s.side]],
                             .line_def = line_def,
                             .front_sector = sides[line_def->side_num[s.side]].sector,
                             .back_sector = ((line_def->flags & core::line_def_flags::two_sided) != 0)
                                                ? sides[line_def->side_num[s.side ^ 1]].sector
                                                : nullptr,
                             .length = length(*v2 - *v1)};
            });
        }

        void group_lines(level_t& level)
        {
            // look up sector number for each subsector
            for (auto& s : level.sub_sectors)
                s.sector = level.segs[s.first_line].side_def->sector;

            // count number of lines in each sector
            for (auto& l : level.lines)
            {
                if (l.front_sector != nullptr) l.front_sector->lines.push_back(&l);

                if (l.back_sector != nullptr && l.front_sector != l.back_sector) l.back_sector->lines.push_back(&l);
            }

            /*todo

            // Generate bounding boxes for sectors

            sector = sectors;
            for (i = 0; i < numsectors; i++, sector++)
            {
                M_ClearBox(bbox);

                for (j = 0; j < sector->linecount; j++)
                {
                    li = sector->lines[j];

                    M_AddToBox(bbox, li->v1->x, li->v1->y);
                    M_AddToBox(bbox, li->v2->x, li->v2->y);
                }

                // set the degenmobj_t to the middle of the bounding box
                sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
                sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

                // adjust bounding box to map blocks
                block                    = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
                block                    = block >= bmapheight ? bmapheight - 1 : block;
                sector->blockbox[BOXTOP] = block;

                block                       = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
                block                       = block < 0 ? 0 : block;
                sector->blockbox[BOXBOTTOM] = block;

                block                      = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
                block                      = block >= bmapwidth ? bmapwidth - 1 : block;
                sector->blockbox[BOXRIGHT] = block;

                block                     = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
                block                     = block < 0 ? 0 : block;
                sector->blockbox[BOXLEFT] = block;
            }
             */
        }
    }

    level_t load_level(core::game_data& data, const rndr::system& renderer, const std::string& lump_name,
                       const std::string& sky_name)
    {
        const auto lump_num = data.lump_table[lump_name];

        level_t lvl;
        lvl.sky_flat_num = data.lump_table.find("F_SKY1")->second;
        lvl.sky_texture = renderer.texture_num(sky_name);
        lvl.blockmap = load_blockmap(data, lump_num + static_cast<size_t>(core::map_lump::blockmap));
        lvl.vertices = load_vertices(data, lump_num + static_cast<size_t>(core::map_lump::vertices));
        lvl.sectors = load_sectors(data, lump_num + static_cast<size_t>(core::map_lump::sectors));
        lvl.sides = load_sides(data, renderer, lvl.sectors, lump_num + static_cast<size_t>(core::map_lump::side_defs));
        lvl.lines =
            load_lines(data, lvl.vertices, lvl.sides, lump_num + static_cast<size_t>(core::map_lump::line_defs));
        lvl.sub_sectors = load_sub_sectors(data, lump_num + static_cast<size_t>(core::map_lump::sub_sectors));
        lvl.nodes = load_nodes(data, lump_num + static_cast<size_t>(core::map_lump::nodes));
        lvl.segs =
            load_segs(data, lvl.vertices, lvl.lines, lvl.sides, lump_num + static_cast<size_t>(core::map_lump::segs));
        group_lines(lvl);

        return lvl;
    }
}