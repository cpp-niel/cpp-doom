#include <core/game_data.hpp>

#include <core/wad_types.hpp>

namespace core
{
    namespace
    {
        // clang-format off
        constexpr auto iwads = std::array{
                iwad_description{.name = "doom.wad", .mission = game_mission::doom, .mode = game_mode::retail, .description = "Doom"},
                iwad_description{.name = "doom1.wad", .mission = game_mission::doom, .mode = game_mode::shareware, .description = "Doom Shareware"},
                iwad_description{.name = "doom2.wad", .mission = game_mission::doom2, .mode = game_mode::commercial, .description = "Doom II"},
        };
        // clang-format on
    }

    void add_wad_file(const std::filesystem::path& iwad_path, core::game_data& data)
    {
        fmt::print(" adding {}\n", iwad_path.string());

        auto wad_file = std::unique_ptr<FILE, decltype(&fclose)>(fopen(iwad_path.string().c_str(), "rbe"), &fclose);
        if (!wad_file) throw std::invalid_argument(fmt::format(" couldn't open {}", iwad_path.string()));

        wad_header header;
        fread(&header, sizeof(wad_header), 1, wad_file.get());
        const auto id = std::string_view(header.identification.data(), 4);
        if ((id != "IWAD") and (id != "PWAD"))
            throw std::invalid_argument(fmt::format("Wad file {} doesn't have IWAD or PWAD id\n", iwad_path.string()));

        auto lump_descriptors = std::vector<wad_lump_descriptor>(header.num_lumps);
        fseek(wad_file.get(), header.info_table_offset, SEEK_SET);
        fread(lump_descriptors.data(), sizeof(wad_lump_descriptor), header.num_lumps, wad_file.get());

        data.lumps.reserve(data.lumps.size() + lump_descriptors.size());
        std::ranges::transform(lump_descriptors, std::back_inserter(data.lumps), [&](const auto& descriptor) {
            return lump_info{.name = array_to_string(descriptor.name),
                             .wad_file = wad_file.get(),
                             .position = descriptor.file_pos,
                             .size = descriptor.size};
        });

        for (auto i = 0; const auto& lump : data.lumps)
            data.lump_table[lump.name] = i++;

        data.wad_files.emplace_back(std::move(wad_file));
    }

    std::pair<std::filesystem::path, iwad_description> find_iwad()
    {
        const auto iwad_dir = std::filesystem::path(ROOT_DIR) / "wad";

        for (const auto& iw : iwads)
        {
            const auto iwad_path = iwad_dir / iw.name;
            if (exists(iwad_path)) return {iwad_path, iw};
        }

        throw std::runtime_error("Game mode indeterminate.  No IWAD file was found.  Try\n"
                                 "specifying one with the '-iwad' command line parameter.\n");
    }
}