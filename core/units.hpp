#pragma once

#include <core/radians.hpp>

#include <cmath>
#include <concepts>
#include <cstdint>

namespace core
{
    class units
    {
    public:
        constexpr units() = default;
        constexpr explicit units(const std::floating_point auto x) : value_(static_cast<real>(x)) {}
        constexpr explicit units(const std::integral auto x) : value_(static_cast<real>(x)) {}

        constexpr auto operator<=>(const units&) const = default;

        template <std::floating_point T>
        constexpr explicit operator T() const noexcept
        {
            return static_cast<T>(value_);
        }

        template <std::integral T>
        constexpr explicit operator T() const noexcept
        {
            return static_cast<T>(value_);
        }

        [[nodiscard]] constexpr units operator-() const
        {
            return units{-value_};
        }

        constexpr units& operator+=(const units x)
        {
            value_ += x.value_;
            return *this;
        }

        constexpr units& operator-=(const units x)
        {
            value_ -= x.value_;
            return *this;
        }

        [[nodiscard]] friend constexpr real operator/(const units v0, const units v1) { return v0.value_ / v1.value_; }

        constexpr units& operator*=(const real scalar)
        {
            value_ *= scalar;
            return *this;
        }

        constexpr units& operator/=(const real scalar)
        {
            value_ /= scalar;
            return *this;
        }

        [[nodiscard]] friend radians atan2(const units y, const units x)
        {
            return radians(std::atan2(y.value_, x.value_));
        }

        [[nodiscard]] friend units abs(const units x)
        {
            return units(std::abs(x.value_));
        }

        [[nodiscard]] friend constexpr real operator*(const units x, const units y)
        {
            return x.value_ * y.value_;
        }

    private:
        real value_{};
    };

    [[nodiscard]] constexpr units operator+(const units v0, const units v1)
    {
        auto result = v0;
        result += v1;
        return result;
    }

    [[nodiscard]] constexpr units operator-(const units v0, const units v1)
    {
        auto result = v0;
        result -= v1;
        return result;
    }

    [[nodiscard]] constexpr units operator*(const units x, const real scalar)
    {
        auto result = x;
        result *= scalar;
        return result;
    }

    [[nodiscard]] constexpr units operator*(const real scalar, const units x)
    {
        return x * scalar;
    }

    [[nodiscard]] constexpr units operator/(const units x, const real scalar)
    {
        auto result = x;
        result /= scalar;
        return result;
    }

    [[nodiscard]] constexpr bool is_zero(const units x)
    {
        constexpr auto epsilon = units{1.0 / 2147483648.0};
        return (x < units{0}) ? -x < epsilon : x < epsilon;
    }

    namespace literals
    {
        [[nodiscard]] constexpr units operator"" _u(const unsigned long long x) { return units(x); }
        [[nodiscard]] constexpr units operator"" _u(const long double x) { return units(x); }
    }
}