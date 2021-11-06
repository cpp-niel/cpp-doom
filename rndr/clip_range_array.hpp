#pragma once

#include <grfx/screen_size.hpp>

#include <algorithm>
#include <array>

namespace rndr
{
    struct clip_range_t
    {
        int first = 0;
        int last = 0;
    };

    class clip_range_array
    {
        using array_t = std::array<clip_range_t, (grfx::max_screen_width / 2) + 1>;
        array_t segs_{};
        array_t::iterator end_{};

    public:
        constexpr clip_range_array() = default;

        constexpr void reset(const int view_width)
        {
            segs_[0].first = -0x7fffffff;
            segs_[0].last = -1;
            segs_[1].first = view_width;
            segs_[1].last = 0x7fffffff;
            end_ = segs_.begin() + 2;
        }

        constexpr void insert(const array_t::iterator pos, const int first, const int last)
        {
            auto it = end_;
            ++end_;

            while (it != pos)
            {
                *it = *(std::prev(it));
                --it;
            }

            it->first = first;
            it->last = last;
        }

        constexpr void remove(const array_t::iterator from, const array_t::iterator to)
        {
            auto left = from;
            auto right = to;
            while (right != end_)
            {
                ++right;
                *left = *right;
                ++left;
            }

            end_ = std::next(left);
        }

        [[nodiscard]] constexpr array_t::iterator first_touching(const int x)
        {
            return std::ranges::find_if(
                    segs_, [&](const int l) { return l >= x; }, &clip_range_t::last);
        }

        [[nodiscard]] constexpr array_t::const_iterator first_touching(const int x) const
        {
            return std::ranges::find_if(
                segs_, [&](const int l) { return l >= x; }, &clip_range_t::last);
        }
    };
}