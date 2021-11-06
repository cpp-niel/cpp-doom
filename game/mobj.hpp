#pragma once

#include <core/vec.hpp>
#include <core/radians.hpp>

namespace game
{
    struct mobj_t
    {
        core::pos position;
        core::units z;
        core::radians angle;

        core::pos mom;
    };
}