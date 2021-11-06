#include <game/arena.hpp>

#include <core/game_data.hpp>
#include <core/math.hpp>
#include <core/vec.hpp>
#include <core/wad_types.hpp>
#include <game/level.hpp>
#include <game/mobj.hpp>
#include <grfx/system.hpp>
#include <rndr/system.hpp>
#include <stdx/overload.hpp>

namespace game
{
    constexpr auto num_keys = 256;
    constexpr auto max_num_players = 4;

    struct player_t
    {
        mobj_t mo;
        // core::units view_z;
        // int num_items = 0;
        // int num_kills = 0;
        // int num_secrets = 0;
    };

    struct arena::impl
    {
        explicit impl(const rndr::system& renderer) : renderer(renderer) {}

        const rndr::system& renderer;
        level_t level;
        std::array<player_t, max_num_players> players{};
        std::array<bool, num_keys> keys_down{};
    };

    namespace
    {
        void thrust(player_t& player, const core::radians angle, const core::units move)
        {
            player.mo.mom.x += move * cos(angle);
            player.mo.mom.y += move * sin(angle);
        }

        const sub_sector_t& sub_sector_containing_point(const level_t& level, const core::pos p)
        {
            // single sub-sector is a special case
            if (level.nodes.empty()) return level.sub_sectors.front();

            auto node_num = level.nodes.size() - 1;

            while ((node_num & core::node_flags::sub_sector) == 0)
            {
                const auto& node = level.nodes[node_num];
                const auto side = core::point_on_side(p, node);
                node_num = node.children[static_cast<int>(side)].index;
            }

            return level.sub_sectors[node_num & ~core::node_flags::sub_sector];
        }

        void spawn_player(const core::map_thing_t& thing, arena::impl& arena_data)
        {
            const auto index = thing.type - 1;
            auto& player = arena_data.players[index];
            player.mo.position = core::pos{.x = core::units(thing.x), .y = core::units(thing.y)};
            player.mo.mom = player.mo.position;
            player.mo.angle = core::radians::from_degrees(static_cast<real>(thing.angle));
        }

        void spawn_map_thing(const core::map_thing_t& thing, arena::impl& arena_data)
        {
            if (thing.type > 0 && thing.type < 5)  // it's a player
                spawn_player(thing, arena_data);
        }

        void load_things(core::game_data& data, const size_t lump, arena::impl& arena_data)
        {
            const auto num_things = lump_size(data, lump) / sizeof(core::map_thing_t);
            const auto things = std::span(cache_lump_num<core::map_thing_t>(data, lump), num_things);

            for (const auto& thing : things)
                spawn_map_thing(thing, arena_data);
        }
    }

    arena::arena(const core::iwad_description& iwad, const start_parameters& parameters, core::game_data& data,
                 const rndr::system& renderer)
        : impl_(std::make_unique<impl>(renderer))
    {
        const auto lump_name = (iwad.mode == core::game_mode::commercial)
                                   ? fmt::format("map{:02}", parameters.map)
                                   : fmt::format("E{}M{}", parameters.episode, parameters.map);

        impl_->level = load_level(data, renderer, lump_name, fmt::format("SKY{}", parameters.episode));
        load_things(data, data.lump_table[lump_name] + static_cast<size_t>(core::map_lump::things), *impl_);
    }

    arena::~arena() = default;

    void arena::draw(grfx::system&, core::game_data& data) const
    {
        impl_->renderer.draw(impl_->level, impl_->players[0].mo, data);
    }

    void arena::tick()
    {
        using namespace ::core::literals;

        constexpr auto move_delta = core::units(0.002);
        constexpr auto turn_delta = 0.1_rad;
        auto forward = core::units(0);
        auto side = core::units(0);
        auto turn = 0_rad;
        if (impl_->keys_down['w']) forward += move_delta;
        if (impl_->keys_down['s']) forward -= move_delta;
        if (impl_->keys_down['n']) side -= move_delta;
        if (impl_->keys_down['m']) side += move_delta;
        if (impl_->keys_down['a']) turn += turn_delta;
        if (impl_->keys_down['d']) turn -= turn_delta;

        impl_->players[0].mo.angle += turn;
        thrust(impl_->players[0], impl_->players[0].mo.angle, forward * 4096);
        thrust(impl_->players[0], impl_->players[0].mo.angle - core::half_pi, side * 2048);

        impl_->players[0].mo.z =
            sub_sector_containing_point(impl_->level, impl_->players[0].mo.mom).sector->floor_height + 41_u;

        impl_->players[0].mo.position = impl_->players[0].mo.mom;
    }

    void arena::handle_event(const core::event_t& e)
    {
        std::visit(stdx::overload{[&](const core::key_down_event& e) { impl_->keys_down[e.us_key_code] = true; },
                                  [&](const core::key_up_event& e) { impl_->keys_down[e.us_key_code] = false; },
                                  [&](const core::quit_event& e) {}},
                   e);
    }
}