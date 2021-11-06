#include <game/system.hpp>

#include <grfx/system.hpp>

namespace game
{
    system::system(const rndr::system& renderer, core::game_data& data, const core::iwad_description& iwad)
        : renderer_(renderer), data_(data), iwad_(iwad)
    {
    }

    void system::new_game(const start_parameters& p) { state_.emplace<arena>(iwad_, p, data_, renderer_); }

    void system::draw(grfx::system& gfx, core::game_data& data) const
    {
        std::visit([&](const auto& s) { s.draw(gfx, data); }, state_);
    }

    void system::tick()
    {
        std::visit([&](auto& s) { s.tick(); }, state_);
    }

    void system::handle_event(const core::event_t& e)
    {
        std::visit([&](auto& s) { s.handle_event(e); }, state_);
    }
}