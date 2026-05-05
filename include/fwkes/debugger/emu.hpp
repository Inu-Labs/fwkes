#pragma once

#include "../bus.h"
#include "../fs.h"

#include <SDL3/SDL.h>

#include <array>
#include <string_view>

#define DEFAULT_FPS 60
#define FRAME_TIME_NS (1000000000 / DEFAULT_FPS)
#define CYCLES_PER_FRAME (CPU_FREQ / DEFAULT_FPS)
#define AUDIO_BUF_SIZE 128 // has to be power of 2!

enum class EmuState {
    Idle,
    Running,
    Paused
};

using Framebuffer = uint32_t[PPU_HEIGHT][PPU_WIDTH];

class Emulator {
  public:
    Emulator(SDL_Renderer *renderer);
    ~Emulator();

    Bus &bus() { return m_bus; }
    const Bus &bus() const { return m_bus; }

    Fs &fs() { return m_fs; }
    const Fs &fs() const { return m_fs; }

    EmuState prev_state() const { return m_prev_state; }

    EmuState state() const { return m_state; }

    void set_state(EmuState state) {
        m_prev_state = m_state;
        m_state = state;
    }

    bool loaded() const { return m_bus.disk_connected; }

    void load_file(std::string_view path);
    void run();
    void stop();
    void pause();
    void resume();
    void step();
    void run_frame();
    void reset();
    void unload();
    void update();
    void render();

    Framebuffer &framebuffer() {
        return m_framebuffer;
    }

    SDL_Texture *canvas() {
        return m_canvas;
    }

    const SDL_Texture *canvas() const {
        return m_canvas;
    }

    uint8_t old_joyplayer_state() const {
        return m_old_joyplayer_state;
    }

    CycleCounter joyplayer_timestamp() const {
        return m_joyplayer_timestamp;
    }

  private:
    void prepare_canvas();
    static void reset_callback(Bus *bus);
    static void render_scanline(Ppu *ppu);
    static void sample_callback(Apu *self, ApuSample smp);

    EmuState m_prev_state;
    EmuState m_state;
    Framebuffer m_framebuffer;
    uint32_t m_colors_lut[32];
    unsigned m_scanline;
    Bus m_bus;
    Fs m_fs;
    ApuSample m_audio_buf[AUDIO_BUF_SIZE];
    unsigned m_sample_count;
    SDL_AudioStream *m_stream;
    SDL_Texture *m_canvas = nullptr;
    bool m_frame_done = false;
    // TODO: delete these and refactor. They're needed for playback recorder.
    uint8_t m_old_joyplayer_state = 0;
    CycleCounter m_joyplayer_timestamp = 0;
};
