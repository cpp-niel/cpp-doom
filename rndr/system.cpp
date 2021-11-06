#include <rndr/system.hpp>

#include <core/wad_types.hpp>
#include <game/level.hpp>
#include <game/mobj.hpp>
#include <grfx/system.hpp>
#include <rndr/bsp_renderer.hpp>
#include <rndr/context.hpp>
#include <rndr/frame.hpp>
#include <rndr/texture_info.hpp>
#include <rndr/view.hpp>
#include <stdx/to.hpp>

#include <algorithm>
#include <span>

namespace rndr
{
    namespace
    {
        using namespace ::core::literals;

        constexpr auto fov = 90_deg;
        constexpr auto num_color_maps = 32;
        constexpr auto dist_map = 2;

        short patch_num_from_name_array(core::game_data& data, const std::array<char, 8>& name_array)
        {
            const auto it = data.lump_table.find(core::array_to_string(name_array));
            return static_cast<short>((it != data.lump_table.end()) ? it->second : -1);
        }

        auto load_patches(const std::vector<short>& patch_nums, const core::map_texture& tex)
        {
            std::vector<texture_patch_t> result(tex.num_patches);
            const auto patches = std::span(&tex.patches[0], tex.num_patches);
            std::ranges::transform(patches, result.begin(), [&](const core::map_patch& p) {
                const auto patch_num = patch_nums[p.patch];
                if (patch_num == -1)
                {
                    throw std::runtime_error(
                        fmt::format("Missing patch in texture {}", core::array_to_string(tex.name)));
                }

                return texture_patch_t{.origin_x = p.origin_x, .origin_y = p.origin_y, .patch = patch_num};
            });

            return result;
        }

        void generate_lookup(core::game_data& data, const int tex_num, texture_info_t& info)
        {
            const auto& texture = info.textures[tex_num];

            // Composited texture not created yet.
            info.composite_size[tex_num] = 0;
            auto& col_lump = info.column_lump[tex_num];
            col_lump.resize(texture.width, 0);
            auto& col_offset = info.column_offset[tex_num];
            col_offset.resize(texture.width, 0);

            // Now count the number of columns
            //  that are covered by more than one patch.
            // Fill in the lump / offset, so columns
            //  with only a single patch are all done.
            auto patch_count = std::vector<std::uint8_t>(texture.width, 0);

            for (const auto& patch : texture.patches)
            {
                const auto& real_patch = *core::cache_lump_num<grfx::patch_t>(data, patch.patch);
                const auto col_offsets = std::span(&real_patch.first_column_offset, real_patch.width);
                const auto x1 = patch.origin_x;
                const auto x2 = std::min(static_cast<short>(x1 + real_patch.width), texture.width);

                for (auto x = std::max(short(0), x1); x < x2; ++x)
                {
                    patch_count[x]++;
                    col_lump[x] = patch.patch;
                    col_offset[x] = col_offsets[x - x1] + 3;
                }
            }

            for (auto x = 0; x < texture.width; x++)
            {
                if (patch_count[x] == 0)
                {
                    fmt::print("generate_lookup: column without a patch ({})\n", texture.name);
                    return;
                }

                if (patch_count[x] > 1)
                {
                    // Use the cached block.
                    col_lump[x] = -1;
                    col_offset[x] = info.composite_size[tex_num];

                    if (info.composite_size[tex_num] > 0x10000 - texture.height)
                        throw std::runtime_error(fmt::format("generate_lookup: texture {} is >64k", tex_num));

                    info.composite_size[tex_num] += texture.height;
                }
            }
        }

        auto offset_to_texture(core::lump_byte_stream& stream, const std::vector<short>& patch_nums)
        {
            return [&](const std::uint32_t offset) {
                stream.set_pos(offset);
                const auto& t = stream.read_span<core::map_texture>(1)[0];
                return texture_t{.name = core::array_to_string(t.name),
                                 .width = t.width,
                                 .height = t.height,
                                 .patches = load_patches(patch_nums, t)};
            };
        }

        void load_textures(const std::vector<short>& patch_nums, core::game_data& data, const std::string_view name,
                           std::vector<texture_t>& result)
        {
            auto stream = core::lump_byte_stream(data, name);
            const auto num_textures = stream.read<std::uint32_t>();
            const auto num_existing = result.size();
            result.resize(num_existing + num_textures);
            const auto offsets = stream.read_span<std::uint32_t>(num_textures);
            std::ranges::transform(offsets, std::next(result.begin(), static_cast<ptrdiff_t>(num_existing)),
                                   offset_to_texture(stream, patch_nums));
        }

        texture_info_t init_textures(core::game_data& data)
        {
            auto names_stream = core::lump_byte_stream(data, "PNAMES");
            const auto num_patches = names_stream.read<std::uint32_t>();
            const auto names = names_stream.read_span<std::array<char, 8>>(num_patches);
            const auto patch_nums =
                names | std::views::transform([&](const auto& name) { return patch_num_from_name_array(data, name); })
                | stdx::to<std::vector>();

            texture_info_t result;
            load_textures(patch_nums, data, "TEXTURE1", result.textures);
            if (data.lump_table.contains("TEXTURE2")) load_textures(patch_nums, data, "TEXTURE2", result.textures);

            result.width_mask.resize(result.textures.size());
            std::ranges::transform(result.textures, result.width_mask.begin(), [](const texture_t& t) {
                int j = 1;
                while (j * 2 <= t.width)
                    j <<= 1;

                return j - 1;
            });

            result.height.resize(result.textures.size());
            std::ranges::transform(result.textures, result.height.begin(),
                                   [](const texture_t& t) { return core::units(t.height); });

            result.composite_size.resize(result.textures.size());
            result.column_lump.resize(result.textures.size());
            result.column_offset.resize(result.textures.size());
            result.composite.resize(result.textures.size());
            result.composite_size.resize(result.textures.size());

            for (auto i = 0; i < result.textures.size(); ++i)
                generate_lookup(data, i, result);

            return result;
        }

        view_t create_view()
        {
            const auto half_view_width = grfx::original_screen_width / 2;
            const auto focal_length = half_view_width / tan(fov * 0.5);
            const auto clip_angle = normalize(core::atan((half_view_width + 1) / focal_length));
            return {.width = grfx::original_screen_width,
                    .height = grfx::original_screen_height,
                    .center_x = grfx::original_screen_width / 2,
                    .center_y = grfx::original_screen_height / 2,
                    .center_x_fraction = core::units(grfx::original_screen_width / 2),
                    .center_y_fraction = core::units(grfx::original_screen_height / 2),
                    .projection = core::units(grfx::original_screen_width / 2),
                    .clip_angle = clip_angle};
        }

        auto color_map_start(const int level)
        {
            return std::clamp(level, 0, num_color_maps - 1) * grfx::palette_size;
        }

        void rebuild_lighting_tables(const view_t& view, const std::span<const light_table_t> color_maps,
                                     lighting_tables_t& tables)
        {
            for (auto i = 0; i < light_levels; i++)
            {
                const auto start_map = ((light_levels - light_bright - i) * 2) * num_color_maps / light_levels;
                for (auto j = 0; j < num_light_scales; j++)
                {
                    const auto level = start_map
                                       - j * grfx::standard_screen_width
                                             / std::min(view.width, grfx::standard_screen_width) / dist_map;

                    tables.scale_light[i][j] = color_maps.subspan(color_map_start(level), grfx::palette_size);
                }
            }
        }
    }

    struct system::impl
    {
        texture_info_t texture_info;
        std::span<const light_table_t> color_maps;

        bool is_view_up_to_date = false;
        view_t view;
        lighting_tables_t lighting_tables;
        bsp_renderer renderer;
    };

    system::system(core::game_data& data) : impl_(std::make_unique<impl>())
    {
        impl_->texture_info = init_textures(data);
        impl_->color_maps = core::cache_lump_as_span<light_table_t>(data, "COLORMAP");

        for (int i = 0; i < light_levels; ++i)
        {
            const auto start_map = ((light_levels - light_bright - i) * 2) * num_color_maps / light_levels;
            for (int j = 0; j < max_light_z; ++j)
            {
                const auto scale = static_cast<int>((0.25 * grfx::original_screen_width) / (j + 1));
                const auto start = color_map_start(start_map - scale);
                impl_->lighting_tables.z_light[i][j] = impl_->color_maps.subspan(start, grfx::palette_size);
            }
        }
    }

    system::~system() = default;

    void system::draw(const game::level_t& level, const game::mobj_t& player, core::game_data& data) const
    {
        if (!impl_->is_view_up_to_date)
        {
            impl_->view = create_view();
            rebuild_lighting_tables(impl_->view, impl_->color_maps, impl_->lighting_tables);
            impl_->is_view_up_to_date = true;
            impl_->renderer.on_view_size_changed(impl_->view.width, impl_->view.height);
        }

        // todo debug only
        std::ranges::fill(grfx::video_buffer, 112);

        const auto frame = frame_t{.position = player.position,
                                   .z = player.z,
                                   .angle = player.angle,
                                   .extra_light = 0};  // todo fill sensibly from player

        std::optional<std::span<const light_table_t>> fixed_color_map;
        //const int fixed_color_map_index = 0;  // todo index of fixed color map should come from player
        //if (fixed_color_map_index > 0)
        //{
        //    fixed_color_map = impl_->color_maps.subspan(fixed_color_map_index * 256, 256);
        //    std::ranges::fill(impl_->lighting_tables.scale_light_fixed, *fixed_color_map);
        //}

        const auto context = context_t{.data = data,
                                       .frame = frame,
                                       .level = level,
                                       .texture_info = impl_->texture_info,
                                       .lighting_tables = impl_->lighting_tables,
                                       .view = impl_->view,
                                       .color_maps = impl_->color_maps,
                                       .fixed_color_map = fixed_color_map};

        impl_->renderer.render_bsp_node(context, static_cast<int>(std::ssize(level.nodes) - 1));
    }

    int system::texture_num(const std::string& name) const
    {
        if (name == "-") return 0;

        const auto it = std::ranges::find(impl_->texture_info.textures, name, &texture_t::name);
        if (it == impl_->texture_info.textures.end())
            throw std::runtime_error(fmt::format("There is no texture called `{}`", name));

        return static_cast<int>(std::distance(impl_->texture_info.textures.begin(), it));
    }
}