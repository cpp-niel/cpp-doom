#pragma once

#include <stdx/iterator_interface.hpp>

#include <cstdint>
#include <ranges>
#include <span>

namespace grfx
{
#pragma pack(push, 1)
    struct patch_t
    {
        short width;              // bounding box width
        short height;             // bounding box height
        short left_offset;        // pixels to the left of origin
        short top_offset;         // pixels below the origin
        int first_column_offset;  // only [width] used the [0] is &column_offsets[width]
    };
#pragma pack(pop)

    struct post_t
    {
        // byte 0 of the post is the top delta which also serves as the end of column marker
        // byte 1 of the post is the length
        constexpr explicit post_t(const std::uint8_t* p) : top_delta(p[0]), pixels(p + 3, p[1]) {}
        std::uint8_t top_delta;
        std::span<const std::uint8_t> pixels;
    };

    class column_view : public std::ranges::view_interface<column_view>
    {
    public:
        constexpr column_view() = default;
        constexpr explicit column_view(const std::uint8_t* p) : ptr_(p) {}

        struct sentinel
        {
            constexpr static auto last_post_in_column_marker = std::uint8_t(0xff);
        };

        class iterator : public stdx::iterator_interface<iterator, std::forward_iterator_tag, post_t, post_t, post_t>
        {
        public:
            constexpr iterator() = default;
            constexpr explicit iterator(const std::uint8_t* p) : ptr_(p) {}
            constexpr auto operator<=>(const iterator&) const = default;
            constexpr post_t operator*() const { return post_t(ptr_); }
            constexpr iterator& operator++()
            {
                ptr_ += ptr_[1] + 4;
                return *this;
            }

            constexpr friend bool operator==([[maybe_unused]] const sentinel& sent, const iterator& iter)
            {
                return *iter.ptr_ == sentinel::last_post_in_column_marker;
            }

            constexpr friend bool operator==(const iterator& iter, const sentinel& sent)
            {
                return sent == iter;
            }

        private:
            const std::uint8_t* ptr_ = nullptr;
        };

        [[nodiscard]] constexpr auto begin() const { return iterator(ptr_); }
        [[nodiscard]] constexpr auto end() const { return sentinel{}; }

    private:
        const std::uint8_t* ptr_ = nullptr;
    };

    class columns : public std::ranges::view_interface<columns>
    {
    public:
        constexpr columns() = default;
        explicit columns(const patch_t& p)
            : column_offsets_(&p.first_column_offset, p.width), ptr_(reinterpret_cast<const std::uint8_t*>(&p))
        {
        }

        template <typename It>
        using iterator_base =
            stdx::iterator_interface<It, std::random_access_iterator_tag, column_view, column_view, column_view>;

        class iterator : public iterator_base<iterator>
        {
        public:
            constexpr iterator() = default;
            constexpr iterator(const columns& parent, const std::span<const int>::iterator it)
                : parent_(&parent), it_(it)
            {
            }

            using iterator_base<iterator>::operator++;
            using iterator_base<iterator>::operator--;

            constexpr auto operator<=>(const iterator&) const = default;
            constexpr column_view operator*() const { return column_view(parent_->ptr_ + *it_); }
            constexpr iterator& operator+=(const std::ptrdiff_t n)
            {
                it_ += n;
                return *this;
            }

            constexpr friend ptrdiff_t operator-(const iterator it0, const iterator it1) { return it0.it_ - it1.it_; }

        private:
            const columns* parent_ = nullptr;
            std::span<const int>::iterator it_;
        };

        [[nodiscard]] constexpr auto begin() const { return iterator(*this, column_offsets_.begin()); }
        [[nodiscard]] constexpr auto end() const { return iterator(*this, column_offsets_.end()); }

    private:
        std::span<const int> column_offsets_;
        const std::uint8_t* ptr_ = nullptr;
    };
}