#pragma once

#include <core/units.hpp>
#include <rndr/clip_range_array.hpp>
#include <rndr/context.hpp>

#include <memory>
#include <vector>

namespace game
{
    struct seg_t;
}

namespace rndr
{
    enum class silhouette_pos
    {
        none,
        bottom,
        top,
        both
    };

    struct draw_seg_t
    {
        const game::seg_t* line = nullptr;
        int x1 = 0;
        int x2 = 0;

        real scale1;
        real scale2;
        real scale_step;

        silhouette_pos silhouette = silhouette_pos::none;

        // do not clip sprites above this
        core::units bsil_height;

        // do not clip sprites below this
        core::units tsil_height;

        // Pointers to lists for sprite clipping,
        //  all three adjusted so [x1] is first value.
        short* sprite_top_clip = nullptr;
        short* sprite_bottom_clip = nullptr;
        short* masked_texture_col = nullptr;
    };

    class bsp_renderer
    {
    public:
        bsp_renderer();
        ~bsp_renderer();

        void render_bsp_node(const context_t& context, const int node_num);
        void on_view_size_changed(const int view_width, const int view_height);

    private:
        struct impl;
        std::unique_ptr<impl> impl_;
    };
}