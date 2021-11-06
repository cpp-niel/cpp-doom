
#include <core/event.hpp>
#include <core/game_data.hpp>
#include <doomkeys.hpp>
#include <game/system.hpp>
#include <grfx/system.hpp>
#include <menu/system.hpp>
#include <rndr/system.hpp>

#include <fmt/format.h>
#include <SDL_filesystem.h>

#include <span>

namespace
{
    int to_us_key_code(const SDL_Keysym& sym)
    {
        int scancode = sym.scancode;
        switch (scancode)
        {
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KEY_RCTRL;

        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KEY_RSHIFT;

        case SDL_SCANCODE_LALT:
            return KEY_LALT;

        case SDL_SCANCODE_RALT:
            return KEY_RALT;

        default:
            return ((scancode >= 0) && (scancode < scancode_to_keys_array.size())) ? scancode_to_keys_array[scancode]
                                                                                   : 0;
        }
    }

    int localized_key_code(const SDL_Keysym& sym)
    {
        const auto result = sym.sym;
        return ((result < 0) || (result >= 128)) ? 0 : result;
    }

    void sdl_key_event(core::event_queue& events, const SDL_Event& e)
    {
        if (e.type == SDL_KEYDOWN)
        {
            const auto us_key_code = to_us_key_code(e.key.keysym);
            if (us_key_code != 0)
            {
                events.push(core::key_down_event{.us_key_code = us_key_code,
                                           .ascii = localized_key_code(e.key.keysym)});
            }
        }
        else if (e.type == SDL_KEYUP)
        {
            const auto us_key_code = to_us_key_code(e.key.keysym);
            if (us_key_code != 0) events.push(core::key_up_event{.us_key_code = us_key_code});
        }
    }

    std::string default_config_dir()
    {
        const auto dir = std::unique_ptr<char, decltype(&SDL_free)>(SDL_GetPrefPath("", PACKAGE_NAME), &SDL_free);
        return dir.get();
    }

    void sdl_get_events(core::event_queue& events)
    {
        SDL_Event sdl_event;

        SDL_PumpEvents();

        while (SDL_PollEvent(&sdl_event) != 0)
        {
            switch (sdl_event.type)
            {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                sdl_key_event(events, sdl_event);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEWHEEL:
                break;

            case SDL_QUIT:
                events.push(core::quit_event());
                break;

                // case SDL_WINDOWEVENT:
                // if (sdl_event.window.windowID == SDL_GetWindowID(screen)) { sdl_window_event(&sdl_event.window); }
                // break;

            default:
                break;
            }
        }
    }

    void process_events(core::event_queue& events, menu::system& menu_sys, game::system& game_sys)
    {
        while (const auto e = events.pop())
        {
            if (!menu_sys.handle_event(*e))
                game_sys.handle_event(*e);
        }
    }
}

int main()
{
    try
    {
        fmt::print("doom++ : a C++ implementation of the original classic first person shooter\n");

        const auto config = core::configuration{.dir = default_config_dir()};
        fmt::print("config dir: {}\n", config.dir);

        // todo load config & bind keys

        // todo -iwad param, build iwad dirs, choose iwad file

        fmt::print("Initializing wad files\n");

        const auto [iwad_path, iwad] = core::find_iwad();

        core::game_data data;
        add_wad_file(iwad_path, data);

        auto rndr_sys = rndr::system(data);

        auto game_sys = game::system(rndr_sys, data, iwad);

        auto menu_sys = menu::system(iwad, game_sys);

        auto gfx_sys = grfx::system({}, data);

        core::event_queue events;
        while (true)
        {
            sdl_get_events(events);

            process_events(events, menu_sys, game_sys);

            game_sys.tick();

            game_sys.draw(gfx_sys, data);

            menu_sys.draw(gfx_sys, data);
            gfx_sys.update();
        }
    }
    catch (std::exception& e)
    {
        fmt::print("Shut down due to error: {}\n", e.what());
        return -1;
    }
    catch (...)
    {
        fmt::print("Shut down due to unknown error\n");
        return -1;
    }
}
