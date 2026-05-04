#pragma once

#include "memory.hpp"
#include "registers.hpp"
#include "../vm.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

namespace ui {
    class Ui {
      public:
        Ui(SDL_Window *win, SDL_Renderer *renderer, Vm &vm);
        ~Ui();

        void update();
        void render();

        bool want_keyboard() const { return m_io->WantCaptureKeyboard; }

        bool quit_requested() const { return m_quit; }

      private:
        void main_menu_bar();

        ImGuiIO *m_io;
        bool m_quit = false;

        bool m_show_demo_window = false;
        Registers m_regs;
        Memory m_memory;

        SDL_Renderer *m_renderer;
        Vm &m_vm;
    };
}
