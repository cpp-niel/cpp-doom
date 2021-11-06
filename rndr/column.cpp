#include <rndr/column.hpp>

#include <grfx/system.hpp>
#include <rndr/context.hpp>
#include <rndr/frame.hpp>
#include <rndr/texture_info.hpp>
#include <rndr/view.hpp>

#include <concepts>

namespace rndr
{
    namespace
    {
        constexpr bool is_power_of_two(const std::integral auto i) { return (i & (i - 1)) == 0; }

        void draw_column_in_cache(const grfx::column_view& col, const std::span<std::uint8_t> cache, const int y)
        {
            for (const auto& post : col)
            {
                const auto raw_target_pos = y + post.top_delta;
                const auto target_pos = std::max(0, raw_target_pos);
                const auto max_pos = std::ssize(cache) - target_pos;
                const auto negative_offset = raw_target_pos - target_pos;
                const auto count = std::clamp(std::ssize(post.pixels) + negative_offset, 0L, max_pos);
                std::ranges::copy(post.pixels.subspan(0, count), cache.data() + target_pos);
            }
        }

        void generate_composite(context_t& context, const int tex_num)
        {
            auto& texture_info = context.texture_info;
            const auto& texture = texture_info.textures[tex_num];

            auto& composite = texture_info.composite[tex_num];
            composite.resize(texture_info.composite_size[tex_num]);

            const auto& offsets = texture_info.column_offset[tex_num];
            const auto target_span = [&, size = texture.height](const int x) {
                return std::span(composite.data() + offsets[x], size);
            };

            const auto column_has_multiple_patches = [&lump = texture_info.column_lump[tex_num]](const int x) {
                return lump[x] < 0;
            };

            // Composite the columns together.
            for (const auto& tex_patch : texture.patches)
            {
                const auto& real_patch = *core::cache_lump_num<grfx::patch_t>(context.data, tex_patch.patch);
                const auto x1 = static_cast<int>(tex_patch.origin_x);
                const auto x2 = std::min(x1 + real_patch.width, static_cast<int>(texture.width));

                for (auto x = std::max(0, x1); const auto& col : grfx::columns(real_patch) | std::views::take(x2 - x))
                {
                    if (column_has_multiple_patches(x)) draw_column_in_cache(col, target_span(x), tex_patch.origin_y);

                    ++x;
                }
            }
        }

        template <bool IsSourceSizePowerOf2>
        void blit_source_column_to_dest(const size_t count, const int mask, const view_t& view, const draw_column_t& dc)
        {
            const auto w = static_cast<size_t>(view.width);
            const auto dest_span = grfx::video_buffer.subspan(dc.y_start * w + dc.x, w * count);
            auto fraction = dc.texture_mid + static_cast<real>(dc.y_start - view.center_y) * dc.fraction_step;
            const auto source_size = static_cast<real>(dc.source.size());
            for (size_t i = 0; i < count; ++i)
            {
                const auto source_index = static_cast<int>(fraction) & mask;
                const auto source = dc.source[source_index];
                dest_span[i * w] = dc.color_map[source];
                fraction += dc.fraction_step;

                if constexpr (IsSourceSizePowerOf2)
                {
                    if (fraction >= source_size) fraction -= source_size;
                }
            }
        }
    }

    std::span<const std::uint8_t> get_column(const context_t& context, const int tex, const unsigned int column)
    {
        const auto& texture = context.texture_info.textures[tex];
        const auto tex_height = static_cast<size_t>(texture.height);
        const auto col = column & context.texture_info.width_mask[tex];
        const auto lump = context.texture_info.column_lump[tex][col];
        const auto ofs = context.texture_info.column_offset[tex][col];

        if (lump > 0) return core::cache_lump_num_as_span<std::uint8_t>(context.data, lump).subspan(ofs, tex_height);

        // todo const_cast
        if (context.texture_info.composite[tex].empty()) generate_composite(const_cast<context_t&>(context), tex);

        return std::span(context.texture_info.composite[tex].data(), ofs + tex_height).subspan(ofs, tex_height);
    }

    void draw_column(const view_t& view, const draw_column_t& dc)
    {
        const auto count = (dc.y_end - dc.y_start) + 1;
        if (count <= 0) return;

        if (is_power_of_two(dc.source.size()))
            blit_source_column_to_dest<true>(count, dc.source.size() - 1, view, dc);
        else
            blit_source_column_to_dest<false>(count, 0xffffffff, view, dc);
    }
}