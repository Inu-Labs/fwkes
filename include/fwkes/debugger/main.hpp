#pragma once

#include "ui/ui.hpp"
#include "vm.hpp"

#include <SDL3/SDL.h>

class Fwkes {
  public:
    Fwkes(SDL_Window *win, SDL_Renderer *renderer);

    void run();

  private:
    void update();
    void render();

    SDL_Renderer *m_renderer;
    bool m_quit = false;

    Vm m_vm;
    ui::Ui m_ui;
};
