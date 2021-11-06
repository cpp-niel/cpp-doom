#pragma once

#include <core/radians.hpp>
#include <core/vec.hpp>

namespace rndr
{
    struct frame_t
    {
        core::pos position;
        core::units z;
        core::radians angle;
        int extra_light = 0;
    };
}