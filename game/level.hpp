#pragma once

#include <core/radians.hpp>
#include <core/vec.hpp>

#include <array>
#include <span>
#include <string>
#include <vector>

namespace core
{
    struct game_data;
}

namespace rndr
{
    class system;
}

namespace game
{
    struct line_t;

    struct blockmap_t
    {
        int width = 0;
        int height = 0;
        std::span<const short> map;
        core::units x;
        core::units y;
    };

    using vertices_t = std::vector<core::pos>;

    struct sector_t
    {
        core::units floor_height;
        core::units ceiling_height;
        int floor_pic = 0;
        int ceiling_pic = 0;
        int light_level = 0;
        short special = 0;
        short tag = 0;

        /* todo
        // 0 = untraversed, 1,2 = sndlines -1
        int sound_traversed;

        // thing that made a sound (or null)
        mobj_t* sound_target;

        // mapblock bounding box for height changes
        int block_box[4];

        // origin for any sounds played by the sector
        degenmobj_t sound_org;

        // if == validcount, already checked
        int valid_count;

        // list of mobjs in sector
        mobj_t* thing_list;

        // thinker_t for reversable actions
        void* special_data;
         */

        std::vector<line_t*> lines;
    };

    using sectors_t = std::vector<sector_t>;

    struct side_t
    {
        // add this to the calculated texture column
        core::units texture_offset;

        // add this to the calculated texture top
        core::units row_offset;

        // Texture indices.
        // We do not maintain names here.
        int top_texture = 0;
        int bottom_texture = 0;
        int mid_texture = 0;

        // Sector the SideDef is facing.
        sector_t* sector = nullptr;
    };

    using sides_t = std::vector<side_t>;

    struct bounding_box_t
    {
        core::units top;
        core::units bottom;
        core::units left;
        core::units right;
    };

    //
    // Move clipping aid for LineDefs.
    //
    enum class slope_type_t
    {
        horizontal,
        vertical,
        positive,
        negative
    };

    struct line_t
    {
        // Vertices, from v1 to v2.
        const core::pos* v1 = nullptr;
        const core::pos* v2 = nullptr;

        // Precalculated v2 - v1 for side checking.
        core::units dx;
        core::units dy;

        // Animation related.
        short flags = 0;
        short special = 0;
        short tag = 0;

        // Visual appearance: SideDefs.
        //  side_num[1] will be -1 (NO_INDEX) if one sided
        std::array<short, 2> side_num{};

        // Neat. Another bounding box, for the extent of the LineDef.
        bounding_box_t bbox;

        // To aid move clipping.
        slope_type_t slope_type = slope_type_t::horizontal;

        // Front and back sector.
        // Note: redundant? Can be retrieved from SideDefs.
        sector_t* front_sector = nullptr;
        sector_t* back_sector = nullptr;

        // if == valid_count, already checked
        int valid_count = 0;

        // thinker_t for reversible actions
        void* special_data = nullptr;
    };

    using lines_t = std::vector<line_t>;

    struct sub_sector_t
    {
        const sector_t* sector = nullptr;
        int num_lines = 0;
        int first_line = 0;
    };

    using sub_sectors_t = std::vector<sub_sector_t>;

    struct node_child_t
    {
        unsigned short index = 0;
        bounding_box_t bbox;
    };

    struct node_t
    {
        // Partition line.
        core::units x;
        core::units y;
        real dx{};
        real dy{};

        std::array<node_child_t, 2> children;
    };

    using nodes_t = std::vector<node_t>;

    struct seg_t
    {
        const core::pos* v1 = nullptr;
        const core::pos* v2 = nullptr;

        core::units offset;

        core::radians angle;

        const side_t* side_def = nullptr;
        const line_t* line_def = nullptr;

        // Sector references.
        // Could be retrieved from line_def, too.
        // back_sector is nullptr for one sided lines
        const sector_t* front_sector = nullptr;
        const sector_t* back_sector = nullptr;

        core::units length;
    };

    using segs_t = std::vector<seg_t>;

    struct level_t
    {
        int sky_flat_num = 0;
        int sky_texture = 0;

        int num_items = 0;
        int num_monsters = 0;
        int num_secrets = 0;

        blockmap_t blockmap;
        vertices_t vertices;
        sectors_t sectors;
        sides_t sides;
        lines_t lines;
        sub_sectors_t sub_sectors;
        nodes_t nodes;
        segs_t segs;
    };

    level_t load_level(core::game_data& data, const rndr::system& renderer, const std::string& lump_name,
                       const std::string& sky_name);
}