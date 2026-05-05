#pragma once

#include "../message.hpp"
#include "../emu.hpp"
#include "canvas.hpp"
#include "disassembler.hpp"
#include "memory.hpp"
#include "registers.hpp"
#include "playback_recorder.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <string>

namespace ui {
    class Ui {
      public:
        Ui(SDL_Window *win, SDL_Renderer *renderer, Emulator &emu,
           MessageQueue &msg_queue);

        ~Ui();

        void update();
        void render();

        void show_error(const std::string &err);
        void request_quit();

        bool want_keyboard() const { return m_io->WantTextInput; }

        Disassembler disassembler;
        Registers regs;
        Memory memory_view;
        PlaybackRecorder playback_recorder;
        Canvas canvas;

      private:
        void input();
        void main_menu_bar();
        void begin_dockspace();
        void end_dockspace();
        void status_bar();
        void tool_bar();
        void quit_popup();
        void error_popup();

        ImGuiIO *m_io;
        std::string m_curr_err;
        bool m_open_quit_popup = false;
        bool m_open_error_popup = false;
        bool m_show_demo_window = false;

        SDL_Renderer *m_renderer;
        Emulator &m_emu;
        MessageQueue &m_msg_queue;
    };
}
