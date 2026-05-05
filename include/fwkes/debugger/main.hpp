#pragma once

#include "message.hpp"
#include "ui/ui.hpp"
#include "emu.hpp"

#include <SDL3/SDL.h>

#include <queue>

class Debugger {
  public:
    Debugger(SDL_Window *win, SDL_Renderer *renderer);

    void run();

  private:
    void update();
    void render();
    void handle_events();
    void handle_messages();
    void update_input();

    SDL_Renderer *m_renderer = nullptr;
    bool m_quit = false;
    std::queue<Message> m_msg_queue;

    Emulator m_emu;
    ui::Ui m_ui;
};
