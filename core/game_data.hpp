#pragma once

#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace core
{
    constexpr int hud_font_start = '!';  // the first font characters
    constexpr int hud_font_end = '_';    // the last font characters

    constexpr auto hud_font_size = hud_font_end - hud_font_start;

    constexpr bool is_in_hud_font(const int c) { return (c >= 0) and (c < hud_font_size); }

    inline int index_in_hud_font(const int c) { return toupper(c) - hud_font_start; }

    enum class game_mission
    {
        doom,
        doom2,
    };

    enum class game_mode
    {
        shareware,
        registered,
        commercial,
        retail,
        unknown
    };

    // Lump order in a map WAD: each map needs a couple of lumps
    // to provide a complete scene geometry description.
    enum class map_lump : size_t
    {
        label,        // A separator, name, ExMx or MAPxx
        things,       // Monsters, items..
        line_defs,    // LineDefs, from editing
        side_defs,    // SideDefs, from editing
        vertices,     // Vertices, edited and BSP splits generated
        segs,         // LineSegs, from LineDefs split by BSP
        sub_sectors,  // SubSectors, list of LineSegs
        nodes,        // BSP nodes
        sectors,      // Sectors, from editing
        reject,       // LUT, sector-sector visibility
        blockmap,     // LUT, motion clipping, walls/grid element
    };

    struct configuration
    {
        std::string dir;
    };

    struct iwad_description
    {
        std::string_view name;
        game_mission mission;
        game_mode mode;
        std::string_view description;
    };

    struct lump_info
    {
        std::string name;
        FILE* wad_file;
        int position;
        int size;
        std::vector<std::byte> cache;
    };

    struct game_data
    {
        std::vector<std::unique_ptr<FILE, decltype(&fclose)>> wad_files;
        std::vector<lump_info> lumps;
        std::unordered_map<std::string, size_t> lump_table;
    };

    inline int lump_size(const core::game_data& data, const size_t num)
    {
        if ((num < 0) || (num >= data.lumps.size()))
            throw std::runtime_error(fmt::format("Lump with index '{}' does not exist", num));

        return data.lumps[num].size;
    }

    template <typename DataType>
    const DataType* cache_lump_num(core::game_data& data, const size_t num)
    {
        auto& lump = data.lumps[num];
        if (lump.cache.empty())
        {
            lump.cache.resize(lump.size);

            fseek(lump.wad_file, lump.position, SEEK_SET);
            fread(lump.cache.data(), lump.size, 1, lump.wad_file);
        }

        return reinterpret_cast<DataType*>(lump.cache.data());
    }

    template <typename DataType>
    const DataType* cache_lump(core::game_data& data, const std::string_view name)
    {
        const auto it = data.lump_table.find(std::string(name));  // todo avoid string construction here
        if (it == data.lump_table.end()) throw std::runtime_error(fmt::format("Lump '{}' could not be found", name));

        return cache_lump_num<DataType>(data, it->second);
    }

    template <typename DataType>
    std::span<const DataType> cache_lump_num_as_span(core::game_data& data, const size_t num)
    {
        return {cache_lump_num<DataType>(data, num), lump_size(data, num) / sizeof(DataType)};
    }

    template <typename DataType>
    std::span<const DataType> cache_lump_as_span(core::game_data& data, const std::string_view name)
    {
        const auto it = data.lump_table.find(std::string(name));  // todo avoid string construction here
        if (it == data.lump_table.end()) throw std::runtime_error(fmt::format("Lump '{}' could not be found", name));

        return cache_lump_num_as_span<DataType>(data, it->second);
    }

    template <size_t N>
    std::string array_to_string(const std::array<char, N>& arr)
    {
        const auto len = std::min(strlen(arr.data()), N);
        std::string result(len, 'x');
        std::transform(arr.data(), arr.data() + len, result.begin(), [](const char c) { return std::toupper(c); });
        return result;
    }

    class lump_byte_stream
    {
    public:
        lump_byte_stream(core::game_data& data, const std::string_view name) : bytes_(cache_lump<std::byte>(data, name))
        {
        }
        lump_byte_stream(core::game_data& data, const size_t num) : bytes_(cache_lump_num<std::byte>(data, num)) {}

        template <typename T>
        T read()
        {
            const auto sz = sizeof(T);
            T result{};
            memcpy(&result, current_address(), sz);
            pos_ += sz;
            return result;
        }

        template <typename T>
        std::span<const T> read_span(const size_t n)
        {
            const auto sz = n * sizeof(T);
            auto result = std::span(reinterpret_cast<const T*>(current_address()), n);
            pos_ += sz;
            return result;
        }

        void set_pos(const size_t pos) { pos_ = pos; }

    private:
        [[nodiscard]] const std::byte* current_address() const { return &bytes_[pos_]; }

        const std::byte* bytes_ = nullptr;
        std::size_t pos_ = 0;
    };

    void add_wad_file(const std::filesystem::path& iwad_path, core::game_data& data);

    std::pair<std::filesystem::path, iwad_description> find_iwad();
}