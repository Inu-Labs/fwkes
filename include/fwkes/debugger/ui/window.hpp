#pragma once

#include "../imgui.hpp"

#include <string>

namespace ui {
    class Window {
      public:
        Window(const char *title, ImGuiWindowFlags flags);
        virtual ~Window();

        void render();

        void show() { m_show = true; }
        void hide() { m_show = false; }

        bool &show_flag() { return m_show; }

        bool hidden() const { return !m_show; }

      private:
        virtual void main() = 0;
        virtual void pre_main();
        virtual void post_main();

        std::string m_title;
        ImGuiWindowFlags m_flags;
        bool m_show = false;
    };
}
