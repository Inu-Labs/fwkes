#include <fwkes/debugger/imgui.hpp>

// Shamelessly stolen from https://github.com/WerWolv/ImHex/blob/98369600c3b80ae2d8922ee01bdbf9dd2a228fc5/lib/libimhex/source/ui/imgui_imhex_extensions.cpp#L823
//
// By the way, one of the best HEX editors out there!
bool ImGuiExt::toolbar_btn(const char *symbol, ImVec4 color) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    color.w = 1.0F;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    const ImGuiID id = window->GetID(symbol);
    const ImVec2 label_size = ImGui::CalcTextSize(symbol, nullptr, true);

    ImVec2 pos = window->DC.CursorPos;

    ImVec2 size = ImGui::CalcItemSize(
        ImVec2(1, 1) * ImGui::GetCurrentWindow()->MenuBarHeight,
        label_size.x + style.FramePadding.x * 2.0F,
        label_size.y + style.FramePadding.y * 2.0F
    );

    ImVec2 padding = (size - label_size) / 2;

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGui::PushStyleColor(ImGuiCol_Text, color);

    // Render
    ImU32 col = 0x00;
    if (held)
        col = ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive);
    else if (hovered)
        col = ImGui::GetColorU32(ImGuiCol_ScrollbarGrabHovered);
    ImGui::RenderNavCursor(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, false, style.FrameRounding);
    ImGui::RenderTextClipped(
        bb.Min + padding, bb.Max - padding, symbol, nullptr, &size,
        style.ButtonTextAlign, &bb
    );

    ImGui::PopStyleColor();

    // Automatically close popups
    // if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) &&
    // (window->Flags & ImGuiWindowFlags_Popup))
    //    CloseCurrentPopup();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, symbol, g.LastItemData.StatusFlags);
    return pressed;
}
