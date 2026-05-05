#pragma once

#include <fonts/codicons.h>

#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <imgui_memory_editor.h>

using Rgb = uint32_t;

namespace ImGuiExt {
    inline ImVec4 rgb_to_imvec4(Rgb color) {
        float s = 1.0f / 255.0f;

        return ImVec4(
            ((color >> IM_COL32_B_SHIFT) & 0xFF) * s,
            ((color >> IM_COL32_G_SHIFT) & 0xFF) * s,
            ((color >> IM_COL32_R_SHIFT) & 0xFF) * s, 1.f
        );
    }

    bool toolbar_btn(const char *symbol, ImVec4 color);

    inline bool toolbar_btn(const char *symbol, Rgb color) {
        return toolbar_btn(symbol, rgb_to_imvec4(color));
    }
}
