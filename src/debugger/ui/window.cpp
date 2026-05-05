#include <fwkes/debugger/ui/window.hpp>

using namespace ui;

Window::Window(const char *title, ImGuiWindowFlags flags)
    : m_title{title}, m_flags{flags} {}

Window::~Window() {}

void Window::render() {
    if (!m_show) {
        return;
    }

    pre_main();

    if (!ImGui::Begin(m_title.c_str(), &m_show, m_flags)) {
        post_main();
        ImGui::End();

        return;
    }

    main();
    post_main();

    ImGui::End();
}

void Window::pre_main() {}

void Window::post_main() {}
