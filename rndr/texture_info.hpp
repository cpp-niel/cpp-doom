#pragma once

#include <core/units.hpp>

#include <string>
#include <vector>

namespace rndr
{
    struct texture_patch_t
    {
        short origin_x = 0;
        short origin_y = 0;
        int patch = 0;
    };

    struct texture_t
    {
        std::string name;
        short width = 0;
        short height = 0;
        std::vector<texture_patch_t> patches;
    };

    struct texture_info_t
    {
        std::vector<texture_t> textures;
        std::vector<int> width_mask;
        std::vector<core::units> height;
        std::vector<std::vector<int>> column_lump;
        std::vector<std::vector<unsigned>> column_offset;
        std::vector<int> composite_size;
        std::vector<std::vector<std::uint8_t>> composite;
    };
}