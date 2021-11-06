#pragma once

#include <concepts>
#include <iterator>
#include <ranges>

namespace stdx
{
    template <std::ranges::input_range Rng, std::movable T, typename Proj = std::identity, typename BinOp>
    constexpr T fold(Rng&& rng, T init, BinOp op, Proj proj = {})
    {
        auto cur = std::ranges::begin(rng);
        const auto last = std::ranges::end(rng);
        for (; cur != last; ++cur)
            init = op(std::move(init), std::invoke(proj, *cur));

        return init;
    }
}