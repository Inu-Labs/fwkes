#include <fwkes/desktop/main.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

using namespace ui;

Fwkes::Fwkes(SDL_Window *win, SDL_Renderer *renderer)
    : m_renderer{renderer}, m_ui{win, renderer, m_vm} {}

void Fwkes::run() {
    while (!m_quit) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);

            switch (ev.type) {
            case SDL_EVENT_QUIT:
                m_quit = true;

                break;
            default:
                break;
            }
        }

        update();
        render();
    }
}

void Fwkes::update() {}

void Fwkes::render() {
    SDL_SetRenderDrawColor(m_renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(m_renderer);

    m_ui.update();
    m_ui.render();

    SDL_RenderPresent(m_renderer);
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }

    SDL_Window *win;
    SDL_Renderer *renderer;

    if (!SDL_CreateWindowAndRenderer(
            "FWKES (desktop development)", 800, 600, 0, &win, &renderer
        )) {
        SDL_Quit();

        return -1;
    }

    Fwkes fwkes(win, renderer);
    fwkes.run();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();

    return 0;
}
