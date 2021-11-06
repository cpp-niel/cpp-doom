#include <grfx/sdl_system.hpp>

namespace grfx
{
    std::span<pixel_t> video_buffer;

    namespace
    {
        auto get_window_position(const int display_index, const int window_width, const int window_height)
        {
            if (display_index < 0 || display_index >= SDL_GetNumVideoDisplays())
            {
                throw std::invalid_argument(fmt::format("The display index {} is not available (must be >= 0 and < {})",
                                                        display_index, SDL_GetNumVideoDisplays()));
            }

            SDL_Rect bounds;
            if (SDL_GetDisplayBounds(display_index, &bounds) < 0)
            {
                throw std::invalid_argument(
                    fmt::format("Could not get display bounds for display index {}", display_index));
            }

            return std::pair(bounds.x + std::max((bounds.w - window_width) / 2, 0),
                             bounds.y + std::max((bounds.h - window_height) / 2, 0));
        }

        void set_window_title(SDL_Window& screen) { SDL_SetWindowTitle(&screen, PACKAGE_NAME); }

        void set_window_icon(SDL_Window& screen)
        {
            const auto surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>(
                SDL_CreateRGBSurfaceFrom((void*)icon_data().data(), icon_width, icon_height, 32, icon_width * 4,
                                         0xffU << 24U, 0xffU << 16U, 0xffU << 8U, 0xffU << 0U),
                &SDL_FreeSurface);

            SDL_SetWindowIcon(&screen, surface.get());
        }
    }

    sdl_library::sdl_library()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error(fmt::format("Failed to initialize video: {}", SDL_GetError()));
    }

    sdl_library::~sdl_library()
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        SDL_Quit();
    }

    sdl_system::sdl_system(const gfx_settings& settings, core::game_data& data)
    {
        blit_rect_.w = screen_size_.width;
        blit_rect_.h = screen_height;

        set_video_mode(display_index_, settings);

        load_and_set_palette(data);

        SDL_Delay(1000);

        SDL_Event dummy;
        while (SDL_PollEvent(&dummy) == 1)
            ;
    }

    void sdl_system::update()
    {
        SDL_LowerBlit(screen_buffer_.get(), &blit_rect_, argb_buffer_.get(), &blit_rect_);
        SDL_UpdateTexture(texture_.get(), nullptr, argb_buffer_->pixels, argb_buffer_->pitch);
        SDL_RenderClear(renderer_.get());

        SDL_SetRenderTarget(renderer_.get(), upscaled_texture_.get());
        SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, nullptr);

        SDL_SetRenderTarget(renderer_.get(), nullptr);
        SDL_RenderCopy(renderer_.get(), upscaled_texture_.get(), nullptr, nullptr);

        SDL_RenderPresent(renderer_.get());
    }

    void sdl_system::create_upscaled_texture()
    {
        int w = 0;
        int h = 0;
        if (SDL_GetRendererOutputSize(renderer_.get(), &w, &h) != 0)
            throw std::runtime_error(fmt::format("Failed to get renderer output size: {}", SDL_GetError()));

        if (w * screen_height < h * screen_size_.width) { h = w * screen_height / screen_size_.width; }
        else
        {
            w = h * screen_size_.width / screen_height;
        }

        const auto w_upscale = std::max((w + screen_size_.width - 1) / screen_size_.width, 1);
        const auto h_upscale = std::max((h + screen_height - 1) / screen_height, 1);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

        upscaled_texture_ =
            sdl_texture_ptr(SDL_CreateTexture(renderer_.get(), pixel_format_, SDL_TEXTUREACCESS_TARGET,
                                              w_upscale * screen_size_.width, h_upscale * screen_height),
                            &SDL_DestroyTexture);
    }

    void sdl_system::set_video_mode(const int display_index, const gfx_settings& settings)
    {
        auto w = window_width;
        auto h = window_height;
        std::uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
        if (settings.is_fullscreen)
        {
            if ((settings.fullscreen_width == 0) and (settings.fullscreen_height == 0))
            { window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; }
            else
            {
                w = settings.fullscreen_width;
                h = settings.fullscreen_height;
                window_flags |= SDL_WINDOW_FULLSCREEN;
            }
        }

        const auto [x, y] = get_window_position(display_index, w, h);

        if (!screen_)
        {
            screen_ = sdl_window_ptr(SDL_CreateWindow(nullptr, x, y, w, h, window_flags), &SDL_DestroyWindow);

            if (!screen_)
            { throw std::runtime_error(fmt::format("Error creating window for video startup: {}", SDL_GetError())); }

            pixel_format_ = SDL_GetWindowPixelFormat(screen_.get());

            SDL_SetWindowMinimumSize(screen_.get(), screen_size_.width, screen_height);

            set_window_title(*screen_);
            set_window_icon(*screen_);
        }

        // The SDL_RENDERER_TARGETTEXTURE flag is required to render the
        // intermediate texture into the upscaled texture.
        std::uint32_t renderer_flags = SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC;

        SDL_DisplayMode mode;
        if (SDL_GetCurrentDisplayMode(display_index, &mode) != 0)
        {
            throw std::runtime_error(
                fmt::format("Could not get display mode for video display {}: {}", display_index, SDL_GetError()));
        }

        if (settings.is_software_renderer)
        {
            renderer_flags |= SDL_RENDERER_SOFTWARE;
            renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;
        }

        renderer_ = sdl_renderer_ptr(SDL_CreateRenderer(screen_.get(), -1, renderer_flags), &SDL_DestroyRenderer);

        if (!renderer_ && !settings.is_software_renderer)
        {
            renderer_flags |= SDL_RENDERER_SOFTWARE;
            renderer_flags &= ~SDL_RENDERER_PRESENTVSYNC;

            renderer_ = sdl_renderer_ptr(SDL_CreateRenderer(screen_.get(), -1, renderer_flags), &SDL_DestroyRenderer);
        }

        if (!renderer_)
            throw std::runtime_error(fmt::format("Error creating renderer for screen window: {}", SDL_GetError()));

        SDL_RenderSetLogicalSize(renderer_.get(), screen_size_.width, screen_height);

        SDL_RenderSetIntegerScale(renderer_.get(), static_cast<SDL_bool>(false));

        SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
        SDL_RenderClear(renderer_.get());
        SDL_RenderPresent(renderer_.get());

        screen_buffer_ = sdl_surface_ptr(SDL_CreateRGBSurface(0, screen_size_.width, screen_height, 8, 0, 0, 0, 0),
                                         &SDL_FreeSurface);
        SDL_FillRect(screen_buffer_.get(), nullptr, 0);

        std::uint32_t rmask = 0;
        std::uint32_t gmask = 0;
        std::uint32_t bmask = 0;
        std::uint32_t amask = 0;
        int unused_bpp = 0;
        SDL_PixelFormatEnumToMasks(pixel_format_, &unused_bpp, &rmask, &gmask, &bmask, &amask);
        argb_buffer_ =
            sdl_surface_ptr(SDL_CreateRGBSurface(0, screen_size_.width, screen_height, 32, rmask, gmask, bmask, amask),
                            &SDL_FreeSurface);
        SDL_FillRect(argb_buffer_.get(), nullptr, 0);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        texture_ = sdl_texture_ptr(SDL_CreateTexture(renderer_.get(), pixel_format_, SDL_TEXTUREACCESS_STREAMING,
                                                     screen_size_.width, screen_height),
                                   &SDL_DestroyTexture);

        create_upscaled_texture();

        video_buffer = std::span(static_cast<pixel_t*>(screen_buffer_->pixels),
                                 static_cast<size_t>(screen_size_.width * screen_height));
    }

    void sdl_system::load_and_set_palette(core::game_data& data)
    {
        using raw_color = std::array<uint8_t, 3>;
        const auto raw_palette = std::span(core::cache_lump<raw_color>(data, "PLAYPAL"), palette_size);
        std::ranges::transform(raw_palette, palette_.begin(),
                               [](const raw_color& c) { return SDL_Color{.r = c[0], .g = c[1], .b = c[2]}; });

        SDL_SetPaletteColors(screen_buffer_->format->palette, palette_.data(), 0, 256);
    }
}