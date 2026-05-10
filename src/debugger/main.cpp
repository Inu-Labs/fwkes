#include <fwkes/debugger/main.hpp>

#include <fwkes/log.h>
#include <fwkes/trace.h>

#include <SDL3/SDL.h>

#include <fwkes/debugger/imgui.hpp>

#include <stdexcept>

using namespace ui;

using KeyBind = std::pair<SDL_Scancode, Button>;
using KeyMap = std::array<KeyBind, 8>;

static const KeyMap g_keymap1 = {
    std::make_pair(SDL_SCANCODE_K, BTN_A),
    std::make_pair(SDL_SCANCODE_J, BTN_B),
    std::make_pair(SDL_SCANCODE_U, BTN_SELECT),
    std::make_pair(SDL_SCANCODE_I, BTN_START),
    std::make_pair(SDL_SCANCODE_W, BTN_UP),
    std::make_pair(SDL_SCANCODE_S, BTN_DOWN),
    std::make_pair(SDL_SCANCODE_A, BTN_LEFT),
    std::make_pair(SDL_SCANCODE_D, BTN_RIGHT)
};

static const KeyMap g_keymap2 = {
    std::make_pair(SDL_SCANCODE_S, BTN_A),
    std::make_pair(SDL_SCANCODE_A, BTN_B),
    std::make_pair(SDL_SCANCODE_Q, BTN_SELECT),
    std::make_pair(SDL_SCANCODE_W, BTN_START),
    std::make_pair(SDL_SCANCODE_UP, BTN_UP),
    std::make_pair(SDL_SCANCODE_DOWN, BTN_DOWN),
    std::make_pair(SDL_SCANCODE_LEFT, BTN_LEFT),
    std::make_pair(SDL_SCANCODE_RIGHT, BTN_RIGHT)
};

static void
update_controller(Joypad &joypad, const KeyMap &keymap, const bool *kbd_state) {
    joypad.state = 0;

    for (size_t i = 0; i < keymap.size(); ++i) {
        if (kbd_state[keymap[i].first]) {
            joypad.state |= keymap[i].second;
        }
    }
}

Debugger::Debugger(SDL_Window *win, SDL_Renderer *renderer)
    : m_renderer{renderer}, m_emu{renderer},
      m_ui{win, renderer, m_emu, m_msg_queue} {
    m_ui.canvas.set_texture(m_emu.canvas());
}

void Debugger::run() {
    while (!m_quit) {
        uint64_t start_time = SDL_GetTicksNS();

        handle_messages();
        handle_events();
        update();
        render();

        uint64_t end_time = start_time + FRAME_TIME_NS;
        uint64_t now = SDL_GetTicksNS();

        if (now < end_time) {
            SDL_DelayPrecise(end_time - now);
        }
    }
}

void Debugger::update() { m_emu.update(); }

void Debugger::render() {
    SDL_SetRenderDrawColor(m_renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(m_renderer);

    m_emu.render();

    m_ui.update();
    m_ui.render();

    SDL_RenderPresent(m_renderer);
}

void Debugger::handle_events() {
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL3_ProcessEvent(&ev);

        switch (ev.type) {
        case SDL_EVENT_QUIT:
            m_msg_queue.emplace(MessageId::Quit);

            break;
        default:
            break;
        }
    }

    if (!m_ui.want_keyboard()) {
        update_input();
    }
}

void Debugger::handle_messages() {
    while (!m_msg_queue.empty()) {
        const Message &msg = m_msg_queue.front();

        switch (msg.id) {
        case MessageId::Quit:
            m_ui.request_quit();

            break;
        case MessageId::ForcedQuit:
            m_quit = true;

            break;
        case MessageId::Error:
            m_ui.show_error(msg.err.msg);

            break;
        case MessageId::Load:
            try {
                m_emu.load_file(msg.load.rom_path);
                m_ui.disassembler.update();
            } catch (const std::runtime_error &err) {
                m_msg_queue.emplace(MessageId::Error, MessageError{err.what()});
            }

            break;
        case MessageId::Unload:
            m_emu.unload();

            break;
        case MessageId::Reload:
            m_emu.reset();
            m_emu.render();

            break;
        case MessageId::Start:
            m_emu.run();

            break;
        case MessageId::Stop:
            m_emu.stop();

            break;
        case MessageId::Pause:
            m_emu.pause();

            break;
        case MessageId::Resume:
            m_emu.resume();

            break;
        case MessageId::StepIn:
            m_emu.step();

            break;
        }

        m_msg_queue.pop();
    }
}

void Debugger::update_input() {
    const bool *kbd_state = SDL_GetKeyboardState(NULL);
    update_controller(m_emu.bus().joypad1, g_keymap1, kbd_state);
    update_controller(m_emu.bus().joypad2, g_keymap2, kbd_state);
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return -1;
    }

    SDL_Window *win;
    SDL_Renderer *renderer;

    if (!SDL_CreateWindowAndRenderer(
            "FWKES Debugger", 1280, 960,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED, &win, &renderer
        )) {
        SDL_Quit();

        return -1;
    }

    trace_add_cat_filter(TRACE_PPU);
    log_add_cat_filter(LOG_PPU);
    log_add_cat_filter(LOG_BUS);

    try {
        Debugger fwkes(win, renderer);
        fwkes.run();
    } catch (const std::exception &err) {
        log_msg(LOG_ERROR, "%s", err.what());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();

    return 0;
}
