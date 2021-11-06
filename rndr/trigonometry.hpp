#pragma once

#include <core/radians.hpp>
#include <core/vec.hpp>
#include <rndr/frame.hpp>

namespace rndr
{
    core::radians point_to_angle(const frame_t& frame, const core::pos pos);
    int view_angle_to_x(const int view_width, const core::radians clip_angle, const core::radians view_angle);
    core::radians x_to_view_angle(const int view_width, const core::radians clip_angle, const int x);
}