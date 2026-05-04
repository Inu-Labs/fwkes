#pragma once

#include <imgui.h>

#include <string>

namespace ui {
    class Window {
      public:
        Window(const char *title, ImGuiWindowFlags flags);
        virtual ~Window();

        void render();

        void show() { m_show = true; }

        void hide() { m_show = false; }

      private:
        virtual void main() = 0;

        std::string m_title;
        ImGuiWindowFlags m_flags;
        bool m_show = false;
    };
}
