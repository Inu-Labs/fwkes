#include <fwkes/debugger/ui/canvas.hpp>

#include <fwkes/debugger/imgui.hpp>

using namespace ui;

Canvas::Canvas(SDL_Texture *texture)
    : Window{"Canvas", 0}, m_texture{texture} {}

void Canvas::main() {
    // Center image, pad it horizontally and maintain same ratio when window is
    // being resized

    float tw, th;
    SDL_GetTextureSize(m_texture, &tw, &th);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float aspect = tw / th;
    float h = avail.y;
    float w = h * aspect;

    if (w > avail.x) {
        w = avail.x;
        h = w / aspect;
    }

    float pad_x = (avail.x - w) * 0.5f;

    if (pad_x < 0.0f) {
        pad_x = 0.0f;
    }

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(cursor.x + pad_x, cursor.y));

    ImGui::Image((ImTextureID) m_texture, ImVec2(w, h));
}

void Canvas::pre_main() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.f));
}

void Canvas::post_main() {
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(1);
}
