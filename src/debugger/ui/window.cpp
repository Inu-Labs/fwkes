#include <fwkes/desktop/ui/window.hpp>

using namespace ui;

Window::Window(const char *title, ImGuiWindowFlags flags)
    : m_title{title}, m_flags{flags} {}

Window::~Window() {}

void Window::render() {
    if (!m_show) {
        return;
    }

    if (!ImGui::Begin(m_title.c_str(), &m_show, m_flags)) {
        ImGui::End();

        return;
    }

    main();

    ImGui::End();
}
