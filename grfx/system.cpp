#include <grfx/system.hpp>

#include <core/game_data.hpp>
#include <core/real.hpp>
#include <grfx/gfx_settings.hpp>
#include <grfx/icon.hpp>
#include <grfx/patch.hpp>
#include <grfx/screen_size.hpp>

namespace grfx
{
    namespace
    {
        constexpr auto lo_to_hi_x(const int x)
        {
            return static_cast<int>(static_cast<real>(standard_screen_width * x) / original_screen_width);
        }

        constexpr auto hi_to_lo_x(const int x)
        {
            return static_cast<real>(original_screen_width * x) / standard_screen_width;
        }

        constexpr auto lo_to_hi_y(const int y)
        {
            return static_cast<int>(static_cast<real>(screen_height * y) / original_screen_height);
        }

        constexpr auto hi_to_lo_y(const int y) { return static_cast<real>(original_screen_height * y) / screen_height; }
    }

    system::system(const gfx_settings& settings, core::game_data& data) : platform_(settings, data)
    {
        raw_video_buffer_ = platform_.raw_video_buffer();
        restore_buffer();
        std::ranges::fill(raw_video_buffer_, 0);

        for (auto j = core::hud_font_start; auto& patch_ptr : hud_font_patches_)
        {
            patch_ptr = core::cache_lump<patch_t>(data, fmt::format("STCFN{:03}", j++));
        }
    }

    void system::draw_patch(const int x_coord, const int y_coord, const patch_t& patch)
    {
        // start and end x in low res coordinates (note start may be off the left of the screen)
        const auto start_x = x_coord - patch.left_offset + screen_size().delta_width;
        const auto end_x = start_x + patch.width;

        // skip any columns that are off the left edge of the screen
        const auto num_cols_to_skip = (start_x < 0) ? -start_x : 0;
        const auto x = start_x + num_cols_to_skip;
        const auto y = y_coord - patch.top_offset;

        // create a span from the row in the target buffer where the top of the patch will be
        const auto lo_to_hi_rows = [&](const int lo_res_y) { return lo_to_hi_y(lo_res_y) * screen_size().width; };
        const auto target_top_row = std::span(target_buffer_.data() + lo_to_hi_rows(y), screen_size().width);

        // iterate through the high-res target columns incrementing the column index in the patch by the
        // fractional width of one high-res pixel in low res coordinates
        const auto cols = columns(patch);
        auto col_index = real(num_cols_to_skip);
        const auto screen_x_end = std::min(lo_to_hi_x(end_x), screen_size().width);
        for (auto screen_x = lo_to_hi_x(x); screen_x < screen_x_end; ++screen_x)
        {
            // for each post ...
            const auto ci = static_cast<int>(col_index);
            col_index += hi_to_lo_x(1);
            for (const auto& post : cols[ci])
            {
                // ...get its position in the target buffer and copy the pixels from the post to the target
                auto* target = &target_top_row[screen_x] + lo_to_hi_rows(post.top_delta);
                for (auto i = real(0); i < real(post.pixels.size()); i += hi_to_lo_y(1))
                {
                    *target = post.pixels[static_cast<int>(i)];
                    target += screen_size().width;
                }
            }
        }
    }

    void system::update() { platform_.update(); }
}