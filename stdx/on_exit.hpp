#pragma once

#include <functional>

namespace stdx
{
    template <typename Func>
    class on_exit
    {
    public:
        explicit on_exit(const Func& f) : func_(f) {}
        ~on_exit() { func_(); }

        on_exit(const on_exit&) = delete;
        on_exit(on_exit&&) = delete;
        on_exit& operator=(const on_exit&) = delete;
        on_exit& operator=(on_exit&&) = delete;

    private:
        Func func_;
    };
}