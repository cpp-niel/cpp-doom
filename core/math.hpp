#pragma once

#include <core/vec.hpp>

namespace core
{
    // clang-format off
    template <typename T>
    concept ray_like = requires(T t)
    {
        { t.x } -> std::convertible_to<core::units>;
        { t.y } -> std::convertible_to<core::units>;
        { t.dx } -> std::convertible_to<real>;
        { t.dy } -> std::convertible_to<real>;
    };
    // clang-format on

    enum class which_side
    {
        front,
        back
    };

    which_side point_on_side(const core::pos p, const ray_like auto& ray)
    {
        const auto side = [](const auto b) { return b ? which_side::back : which_side::front; };

        if (ray.dx == 0)
            return (p.x <= ray.x) ? side(ray.dy > 0) : side(ray.dy < 0);

        if (ray.dy == 0)
            return (p.y <= ray.y) ? side(ray.dx < 0) : side(ray.dx > 0);

        const auto dx = p.x - ray.x;
        const auto dy = p.y - ray.y;

        const auto left = dx * static_cast<real>(ray.dy);
        const auto right = dy * static_cast<real>(ray.dx);

        return side(right >= left);
    }
}