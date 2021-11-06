#include <rndr/trigonometry.hpp>

#include <algorithm>

namespace rndr
{
    core::radians point_to_angle(const frame_t& frame, const core::pos p)
    {
        auto d = p - frame.position;
        return atan2(d.y, d.x);
    }

    int view_angle_to_x(const int view_width, const core::radians clip_angle, const core::radians view_angle)
    {
        const auto half_view_width = view_width * 0.5;
        const auto focal_length = tan(clip_angle) * half_view_width;
        const auto offset_from_center = tan(view_angle) * focal_length;
        const auto x = static_cast<int>(half_view_width - offset_from_center + 1.0);
        return std::clamp(x, 0, view_width);
    }

    core::radians x_to_view_angle(const int view_width, const core::radians clip_angle, const int x)
    {
        const auto half_view_width = view_width * 0.5;
        const auto focal_length = tan(clip_angle) * half_view_width;
        return normalize(core::atan((half_view_width - x) / focal_length));
    }
}