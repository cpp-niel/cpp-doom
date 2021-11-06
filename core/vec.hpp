#pragma once

#include <core/units.hpp>

namespace core
{
    struct pos
    {
        units x{};
        units y{};
    };

    struct vec
    {
        units x{};
        units y{};
    };

    [[nodiscard]] constexpr pos operator+(const pos p, const vec v)
    {
        return { p.x + v.x, p.y + v.y };
    }

    [[nodiscard]] constexpr pos operator-(const pos p, const vec v)
    {
        return { p.x - v.x, p.y - v.y };
    }

    [[nodiscard]] constexpr vec operator-(const pos p0, const pos p1)
    {
        return { p0.x - p1.x, p0.y - p1.y };
    }

    [[nodiscard]] constexpr vec operator*(const vec v, const real scalar)
    {
        return { v.x * scalar, v.y * scalar };
    }

    [[nodiscard]] constexpr vec operator*(const real scalar, const vec v)
    {
        return { v.x * scalar, v.y * scalar };
    }

    [[nodiscard]] constexpr vec operator/(const vec v, const real scalar)
    {
        return { v.x / scalar, v.y / scalar };
    }

    [[nodiscard]] constexpr units dot(const vec v0, const vec v1)
    {
        return units(v0.x * v1.x + v0.y * v1.y);
    }

    [[nodiscard]] inline units length(const vec v)
    {
        return units(std::sqrt(v.x * v.x + v.y * v.y));
    }

    [[nodiscard]] inline vec normalize(const vec v)
    {
        return v / std::sqrt(v.x * v.x + v.y * v.y);
    }

    [[nodiscard]] inline vec unit_normal(const vec v)
    {
        return normalize({v.y, -v.x});
    }

    [[nodiscard]] inline units distance_from_point_to_point(const pos p0, const pos p1)
    {
        return length(p1 - p0);
    }

    [[nodiscard]] inline units distance_from_point_to_line(const pos p, const pos p0, const pos p1)
    {
        return dot(p - p0, unit_normal(p1 - p0));
    }
}