#pragma once

#include <core/real.hpp>

#include <cmath>
#include <concepts>
#include <numbers>

namespace core
{
    class radians
    {
    public:
        radians() = default;
        constexpr explicit radians(const std::floating_point auto val) : value_(static_cast<real>(val)) {}

        constexpr auto operator<=>(const radians&) const = default;

        constexpr radians& operator+=(const radians rad)
        {
            value_ += rad.value_;
            return *this;
        }

        constexpr radians& operator-=(const radians rad)
        {
            value_ -= rad.value_;
            return *this;
        }

        constexpr radians& operator*=(const std::floating_point auto scalar)
        {
            value_ *= static_cast<real>(scalar);
            return *this;
        }

        [[nodiscard]] friend real sin(const radians x) { return std::sin(x.value_); }
        [[nodiscard]] friend real cos(const radians x) { return std::cos(x.value_); }
        [[nodiscard]] friend real tan(const radians x) { return std::tan(x.value_); }
        [[nodiscard]] friend radians fmod(const radians x, const radians y) { return radians(std::fmod(x.value_, y.value_)); }
        [[nodiscard]] friend real to_fraction(const radians x) { return x.value_ / (real{2} * std::numbers::pi_v<real>); }

        [[nodiscard]] static constexpr radians from_doom_angle(const std::integral auto x)
        {
            const auto raw = static_cast<double>(static_cast<unsigned int>(x) << 16);
            return radians(std::numbers::pi * (raw / 2147483647.0));
        }

        [[nodiscard]] static constexpr radians from_degrees(const real x)
        {
            return radians(x / real{180} * std::numbers::pi_v<real>);
        }

    private:
        real value_{};
    };

    constexpr radians operator+(const radians v0, const radians v1)
    {
        auto result = v0;
        result += v1;
        return result;
    }

    constexpr radians operator-(const radians v0, const radians v1)
    {
        auto result = v0;
        result -= v1;
        return result;
    }

    constexpr radians operator*(const radians v, const std::floating_point auto scalar)
    {
        auto result = v;
        result *= scalar;
        return result;
    }

    constexpr radians operator*(const std::floating_point auto scalar, const radians v) { return v * scalar; }

    constexpr auto pi = radians(std::numbers::pi_v<real>);
    constexpr auto half_pi = pi * 0.5;
    constexpr auto two_pi = pi * 2.0;

    [[nodiscard]] inline radians atan(const std::floating_point auto x)
    {
        return radians(std::atan(x));
    }

    [[nodiscard]] inline radians atan2(const std::floating_point auto y, const std::floating_point auto x)
    {
        return radians(std::atan2(y, x));
    }

    [[nodiscard]] inline radians normalize(const radians x)
    {
        auto nx = fmod(x, two_pi);
        return (nx < radians(0.0)) ? nx + two_pi : nx;
    }


    namespace literals
    {
        constexpr radians operator"" _rad(const unsigned long long int val) { return radians(static_cast<real>(val)); }

        constexpr radians operator"" _rad(const long double val) { return radians(static_cast<real>(val)); }

        constexpr radians operator"" _deg(const unsigned long long int val) { return radians::from_degrees(static_cast<real>(val)); }

        constexpr radians operator"" _deg(const long double val) { return radians::from_degrees(static_cast<real>(val)); }
    }
}