#pragma once

#include <array>

namespace core
{
    namespace line_def_flags
    {
        // Solid, is an obstacle.
        constexpr auto blocking = 1;

        // Blocks monsters only.
        constexpr auto block_monsters = 2;

        // Backside will not be present at all
        //  if not two sided.
        constexpr auto two_sided = 4;

        // If a texture is pegged, the texture will have
        // the end exposed to air held constant at the
        // top or bottom of the texture (stairs or pulled
        // down things) and will move with a height change
        // of one of the neighbor sectors.
        // Unpegged textures always have the first row of
        // the texture at the top pixel of the line for both
        // top and bottom textures (use next to windows).

        // upper texture unpegged
        constexpr auto dont_peg_top = 8;

        // lower texture unpegged
        constexpr auto dont_peg_bottom = 16;

        // In AutoMap: don't map as two sided: IT'S A SECRET!
        constexpr auto secret = 32;

        // Sound rendering: don't let sound cross two of these.
        constexpr auto sound_block = 64;

        // Don't draw on the automap at all.
        constexpr auto dont_draw = 128;

        // Set if already seen, thus drawn in automap.
        constexpr auto mapped = 256;
    }

    namespace node_flags
    {
        constexpr unsigned short sub_sector = 0x8000;
        constexpr unsigned short none = 0xFFFF;
    }

#pragma pack(push, 1)
    struct wad_header
    {
        std::array<char, 4> identification{};
        int num_lumps = 0;
        int info_table_offset = 0;
    };

    struct wad_lump_descriptor
    {
        int file_pos = 0;
        int size = 0;
        std::array<char, 8> name{};
    };

    struct map_vertex
    {
        short x = 0;
        short y = 0;
    };

    struct map_sector
    {
        short floor_height = 0;
        short ceiling_height = 0;
        std::array<char, 8> floor_pic{};
        std::array<char, 8> ceiling_pic{};
        short light_level = 0;
        short special = 0;
        short tag = 0;
    };

    struct map_side_def
    {
        short texture_offset = 0;
        short row_offset = 0;
        std::array<char, 8> top_texture;
        std::array<char, 8> bottom_texture;
        std::array<char, 8> mid_texture;
        // Front sector, towards viewer.
        short sector = 0;
    };

    struct map_patch
    {
        short origin_x = 0;
        short origin_y = 0;
        short patch = 0;
        short step_dir = 0;
        short colormap = 0;
    };

    struct map_texture
    {
        std::array<char, 8> name{};
        int masked = 0;
        short width = 0;
        short height = 0;
        int obsolete = 0;
        short num_patches = 0;
        map_patch patches[1];
    };

    struct map_line_def
    {
        short v1 = 0;
        short v2 = 0;
        short flags = 0;
        short special = 0;
        short tag = 0;
        // side_num[1] will be -1 (NO_INDEX) if one sided
        std::array<short, 2> side_num{};
    };

    struct map_sub_sector
    {
        short num_segs = 0;
        short first_seg = 0;
    };

    struct map_node_t
    {
        // Partition line from (x,y) to (x+dx,y+dy)
        short x = 0;
        short y = 0;
        short dx = 0;
        short dy = 0;

        // Bounding box for each child,
        // clip against view frustum.
        std::array<std::array<short, 4>, 2> bboxes{};

        // If NF_SUBSECTOR its a subsector,
        // else it's a node of another subtree.
        std::array<unsigned short, 2> children{};
    };

    struct map_seg_t
    {
        short v1 = 0;
        short v2 = 0;
        short angle = 0;
        short line_def = 0;
        short side = 0;
        short offset = 0;
    };

    struct map_thing_t
    {
        short x = 0;
        short y = 0;
        short angle = 0;
        short type = 0;
        short options = 0;
    };
#pragma pack(pop)
}