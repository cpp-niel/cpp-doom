#pragma once

#include <array>
#include <optional>
#include <type_traits>
#include <variant>

namespace core
{
    struct quit_event
    {
    };

    struct key_down_event
    {
        int us_key_code = 0;
        int ascii = 0;
    };

    struct key_up_event
    {
        int us_key_code = 0;
    };

    template <typename T>
    concept event_type =
        std::is_same_v<T, quit_event> or std::is_same_v<T, key_down_event> or std::is_same_v<T, key_up_event>;

    using event_t = std::variant<quit_event, key_down_event, key_up_event>;

    class event_queue
    {
    public:
        template <event_type Event>
        void push(Event&& e)
        {
            get(head_) = e;
        }

        std::optional<event_t> pop()
        {
            if (head_ == tail_) return std::nullopt;

            return get(tail_);
        }

    private:
        event_t& get(size_t& index)
        {
            auto& result = events_[index];
            index = ((index + 1) % events_.size());
            return result;
        }

        std::array<event_t, 64> events_;
        size_t head_ = 0;
        size_t tail_ = 0;
    };
}