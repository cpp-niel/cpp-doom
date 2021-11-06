#pragma once

#include <type_traits>

namespace stdx
{
    template <typename Iterator, typename Category, typename ValueType, typename Reference = ValueType&,
              typename Pointer = ValueType*>
    class iterator_interface
    {
        template <typename It>
        decltype(auto) access(It it)
        {
            if constexpr (std::is_pointer_v<pointer>)
                return std::addressof(*it.iter());
            else
                return Pointer(*it.iter());
        }

    public:
        using iterator_category [[maybe_unused]] = Category;
        using value_type [[maybe_unused]] = std::remove_const_t<ValueType>;
        using reference [[maybe_unused]] = Reference;
        using pointer = Pointer;
        using difference_type = std::ptrdiff_t;

        constexpr decltype(auto) iter() { return static_cast<Iterator&>(*this); }
        constexpr decltype(auto) iter() const { return static_cast<const Iterator&>(*this); }

        constexpr auto operator->() requires requires { *iter(); }
        {
            return access(*this);
        }

        constexpr auto operator->() const requires requires { *iter(); }
        {
            return access(*this);
        }

        constexpr decltype(auto) operator[](difference_type n) const
            requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            auto it = iter();
            it += n;
            return *it;
        }

        constexpr decltype(auto) operator++() requires(!std::derived_from<Category, std::random_access_iterator_tag>)
        {
            ++iter();
            return iter();
        }

        constexpr decltype(auto) operator++() requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            return iter() += 1;
        }

        constexpr auto operator++(int)
        {
            const auto it = iter();
            ++iter();
            return it;
        }

        constexpr decltype(auto)
        operator+=(difference_type n) requires(!std::derived_from<Category, std::random_access_iterator_tag>)
        {
            iter() += n;
            return iter();
        }

        friend constexpr auto
        operator+(Iterator it, difference_type n) requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            it += n;
            return it;
        }

        friend constexpr auto
        operator+(difference_type n, Iterator it) requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            it += n;
            return it;
        }

        constexpr decltype(auto) operator--() requires std::same_as<Category, std::bidirectional_iterator_tag>
        {
            --iter();
            return iter();
        }

        constexpr decltype(auto) operator--() requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            return iter() += -1;
        }

        constexpr auto operator--(int) requires std::derived_from<Category, std::bidirectional_iterator_tag>
        {
            const auto it = iter();
            --iter();
            return it;
        }

        constexpr decltype(auto)
        operator-=(difference_type n) requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            return iter() += -n;
        }

        friend constexpr auto operator-(Iterator lhs,
                                        Iterator rhs) requires std::same_as<Category, std::bidirectional_iterator_tag>
        {
            return lhs - rhs;
        }

        friend constexpr auto
        operator-(Iterator it, difference_type n) requires std::derived_from<Category, std::random_access_iterator_tag>
        {
            it += -n;
            return it;
        }

        constexpr auto operator<=>(const iterator_interface&) const = default;
    };
}