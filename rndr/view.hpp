#pragma once

#include <core/radians.hpp>
#include <core/units.hpp>

namespace rndr
{
    struct view_t
    {
        int width = 0;
        int height = 0;

        int center_x = 0;
        int center_y = 0;
        core::units center_x_fraction;
        core::units center_y_fraction;

        core::units projection;

        core::radians clip_angle;
    };
}