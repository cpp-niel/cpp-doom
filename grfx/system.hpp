#pragma once

#include <grfx/sdl_system.hpp> // todo pimpl & get this header out of here

#include <span>

namespace grfx
{
    class system
    {
    public:
        system(const gfx_settings& settings, core::game_data& data);

        void draw_patch(const int x_coord, const int y_coord, const patch_t& patch);

        void update();

        const patch_t& hud_font_patch(const int chr) const { return *hud_font_patches_[chr]; }

        auto screen_size() const { return platform_.screen_size(); }

    private:
        sdl_system platform_;

        std::span<pixel_t> raw_video_buffer_;
        std::span<pixel_t> target_buffer_;

        std::array<const patch_t*, core::hud_font_size> hud_font_patches_{};

        void use_buffer(std::span<pixel_t> buffer) { target_buffer_ = buffer; }
        void restore_buffer() { target_buffer_ = raw_video_buffer_; }
    };
}
