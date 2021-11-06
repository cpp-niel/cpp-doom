#pragma once

#include <core/game_data.hpp>
#include <grfx/gfx_settings.hpp>
#include <grfx/grfx.hpp>
#include <grfx/icon.hpp>
#include <grfx/patch.hpp>
#include <grfx/screen_size.hpp>
#include <core/units.hpp>

#include <SDL.h>
#include <SDL_filesystem.h>
#include <fmt/format.h>

#include <span>

namespace grfx
{
    struct sdl_library
    {
        sdl_library();
        ~sdl_library();
    };

    class sdl_system
    {
    public:
        sdl_system(const gfx_settings& settings, core::game_data& data);

        auto raw_video_buffer()
        {
            return std::span(static_cast<pixel_t*>(screen_buffer_->pixels), screen_size_.width * screen_height);
        }

        void update();

        [[nodiscard]] adjusted_screen_size screen_size() const { return screen_size_; }

    private:
        sdl_library sdl_;
        int display_index_ = 0;
        adjusted_screen_size screen_size_;
        std::array<SDL_Color, palette_size> palette_{};

        using sdl_window_ptr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
        using sdl_renderer_ptr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
        using sdl_surface_ptr = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
        using sdl_texture_ptr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

        sdl_window_ptr screen_{nullptr, &SDL_DestroyWindow};
        sdl_renderer_ptr renderer_{nullptr, &SDL_DestroyRenderer};
        sdl_surface_ptr screen_buffer_{nullptr, &SDL_FreeSurface};
        sdl_surface_ptr argb_buffer_{nullptr, &SDL_FreeSurface};
        sdl_texture_ptr texture_{nullptr, &SDL_DestroyTexture};
        sdl_texture_ptr upscaled_texture_{nullptr, &SDL_DestroyTexture};
        std::uint32_t pixel_format_ = 0;
        SDL_Rect blit_rect_ = {0, 0, standard_screen_width, screen_height};

        void create_upscaled_texture();
        void set_video_mode(const int display_index_, const gfx_settings& settings);
        void load_and_set_palette(core::game_data& data);
    };
}
