#include <fwkes/debugger/emu.hpp>

#include <fwkes/joyplayer_demo_events.h>

#include <cstring>
#include <stdexcept>

// clang-format off
static uint32_t g_colors[64] = {
    /*          0         1        2          3         4         5         6         7         8         9         A         B         C         D         E         F */
    /* 0 */ 0x626262ff, 0x001c95ff, 0x1904acff, 0x42009dff, 0x61006bff, 0x6e0025ff, 0x650500ff, 0x491e00ff, 0x223700ff, 0x004900ff, 0x004f00ff, 0x004816ff, 0x00355eff, 0x000000ff, 0x000000ff, 0x000000ff,
    /* 1 */ 0xabababff, 0x0c4edbff, 0x3d2effff, 0x7115f3ff, 0x9b0bb9ff, 0xb01262ff, 0xa92704ff, 0x894600ff, 0x576600ff, 0x237f00ff, 0x008900ff, 0x008332ff, 0x006d90ff, 0x000000ff, 0x000000ff, 0x000000ff,
    /* 2 */ 0xffffffff, 0x57a5ffff, 0x8287ffff, 0xb46dffff, 0xdf60ffff, 0xf863c6ff, 0xf8746dff, 0xde9020ff, 0xb3ae00ff, 0x81c800ff, 0x56d522ff, 0x3dd36fff, 0x3ec1c8ff, 0x4e4e4eff, 0x000000ff, 0x000000ff,
    /* 3 */ 0xffffffff, 0xbee0ffff, 0xcdd4ffff, 0xe0caffff, 0xf1c4ffff, 0xfcc4efff, 0xfdcaceff, 0xf5d4afff, 0xe6df9cff, 0xd3e99aff, 0xc2efa8ff, 0xb7efc4ff, 0xb6eae5ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff
};
// clang-format on

static std::string bios_path(Fs *fs) {
    char cwd[256];
    fs_pwd(fs, cwd, 255);

    return std::string(cwd) + "/bios.nes";
}

Emulator::Emulator(SDL_Renderer *renderer) {
    m_canvas = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256, 240
    );

    if (!m_canvas) {
        throw std::runtime_error(
            std::string("cannot create canvas: ") + SDL_GetError()
        );
    }

    SDL_SetTextureScaleMode(m_canvas, SDL_SCALEMODE_NEAREST);

    const SDL_AudioSpec spec = {SDL_AUDIO_S16, 1, SAMPLE_RATE};
    m_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL
    );

    if (!m_stream) {
        std::string sdl_err = SDL_GetError();
        SDL_DestroyTexture(m_canvas);
        throw std::runtime_error("cannot open audio device stream: " + sdl_err);
    }

    SDL_ResumeAudioStreamDevice(m_stream);

    set_state(EmuState::Idle);
    fs_init(&m_fs);
    bus_init(&m_bus, &m_fs, bios_path(&m_fs).c_str());

    m_bus.reset_cb = &Emulator::reset_callback;
    m_bus.cpu.halt_on_brk = false;
    m_bus.ppu.colors = g_colors;
    m_bus.ppu.scanline_cb = &Emulator::render_scanline;
    m_bus.ppu.user_data = this;
    m_bus.user_data = this;
    m_bus.apu.sample_cb = &Emulator::sample_callback;
    m_bus.apu.user_data = this;
    ppu_init_pixel_luts(&m_bus.ppu);

    joyplayer_init(&m_bus.joyplayer, g_demo_events, g_demo_count);
    m_bus.joyplayer.active = true;
}

Emulator::~Emulator() {
    SDL_DestroyAudioStream(m_stream);
    SDL_DestroyTexture(m_canvas);
    unload();
    bus_deinit(&m_bus);
}

void Emulator::load_file(std::string_view path) {
    if (!bus_load_disk(&m_bus, path.data())) {
        throw std::runtime_error(
            std::string("ROM file '") + path.data() +
            "' cannot be loaded: bad iNES/NES 2.0 header, does not exist or "
            "bad permissions"
        );
    }

    set_state(EmuState::Idle);
}

void Emulator::reset() {
    BusEvent new_ev = {
        .id = BUS_EVENT_RESET,
    };

    bus_add_event(&m_bus, &new_ev);
}

void Emulator::unload() { bus_unload_disk(&m_bus); }

void Emulator::run() { set_state(EmuState::Running); }

void Emulator::stop() {
    reset();
    set_state(EmuState::Idle);
}

void Emulator::pause() {
    if (m_state == EmuState::Running) {
        set_state(EmuState::Paused);
    }
}

void Emulator::resume() {
    if (m_state == EmuState::Paused) {
        set_state(m_prev_state);
    }
}

void Emulator::step() {
    Cpu *cpu = &m_bus.cpu;
    Ppu *ppu = &m_bus.ppu;
    Apu *apu = &m_bus.apu;

    if (m_bus.joyplayer.active) {
        m_old_joyplayer_state = m_bus.joyplayer.current_buttons;
        joyplayer_update(&m_bus.joyplayer, &m_bus);
        m_joyplayer_timestamp = m_bus.cpu.cycles;
        m_bus.joypad1.state = m_bus.joyplayer.current_buttons;
    }

    bus_update(&m_bus);
    cpu_run_until(cpu, cpu->cycles + 1);
    ppu_run_until(ppu, cpu->cycles * 3);
    apu_run_until(apu, cpu->cycles);
}

#define DOTS_PER_SCANLINE 341
#define CYCLES_PER_SCANLINE 113

void Emulator::run_frame() {
    Cpu *cpu = &m_bus.cpu;
    Ppu *ppu = &m_bus.ppu;
    Apu *apu = &m_bus.apu;

    if (m_bus.joyplayer.active) {
        m_old_joyplayer_state = m_bus.joyplayer.current_buttons;
        joyplayer_update(&m_bus.joyplayer, &m_bus);
        m_joyplayer_timestamp = m_bus.cpu.cycles;
        m_bus.joypad1.state = m_bus.joyplayer.current_buttons;
    }

    bus_update(&m_bus);

    while (!ppu->frame_done) {
        // CPU will run until some significant event (vblank/nmi or end of
        // frame)
        CycleCounter curr_ppu_abs_dot =
            ppu->scanline * DOTS_PER_SCANLINE + ppu->dot;
        CycleCounter target_ppu_abs_dot;

        if (m_bus.disk.mapper_hsync && ppu->scanline < 240) {
            if (ppu->dot < 260) {
                // Target the current scanline's IRQ dot
                target_ppu_abs_dot = ppu->scanline * DOTS_PER_SCANLINE + 260;
            } else {
                // We passed it, target the NEXT scanline's IRQ dot
                target_ppu_abs_dot =
                    (ppu->scanline + 1) * DOTS_PER_SCANLINE + 260;
            }
        } else if (ppu->scanline < 241) {
            // next event is vblank
            target_ppu_abs_dot = 241 * DOTS_PER_SCANLINE + 1;
        } else {
            // next event is end of frame
            target_ppu_abs_dot = 262 * DOTS_PER_SCANLINE;
        }

        CycleCounter cycles_to_event =
            (target_ppu_abs_dot - curr_ppu_abs_dot) / 3;
        CycleCounter event_deadline = cpu->cycles + cycles_to_event;
        CycleCounter cycles_to_run = cpu->cycles + CYCLES_PER_SCANLINE;

        // significant event has higher priority
        if (event_deadline < cycles_to_run) {
            cycles_to_run = event_deadline;
        }

        // prevent infinite loop
        if (cycles_to_run <= cpu->cycles) {
            cycles_to_run = cpu->cycles + 1;
        }

        cpu_run_until(cpu, cycles_to_run);
        ppu_run_until(ppu, cpu->cycles * 3);
        apu_run_until(apu, cpu->cycles);
    }

    ppu->frame_done = false;
}

void Emulator::update() {
    if (m_state == EmuState::Running) {
        run_frame();
    }
}

void Emulator::render() {
    if (m_frame_done) {
        SDL_UpdateTexture(
            m_canvas, NULL, m_framebuffer, PPU_WIDTH * sizeof(uint32_t)
        );

        m_frame_done = false;
    }
}

void Emulator::render_scanline(Ppu *ppu) {
    Emulator *emu = (Emulator *) ppu->user_data;

    if (ppu->colors_lut_dirty) {
        memcpy(emu->m_colors_lut, ppu->colors_lut, 32 * sizeof(PpuPixel));
        ppu->colors_lut_dirty = false;
    }

    for (int i = 0; i < PPU_WIDTH; ++i) {
        uint8_t px = ppu->scanline_buf[i];
        emu->m_framebuffer[emu->m_scanline][i] = emu->m_colors_lut[px & 0x1f];
    }

    if (++emu->m_scanline == 240) {
        emu->m_frame_done = true;
        emu->m_scanline = 0;
    }
}

void Emulator::reset_callback(Bus *bus) {
    printf("here\n");
    Emulator *emu = (Emulator *) bus->user_data;

    memset(emu->m_framebuffer, 0, sizeof(emu->m_framebuffer));
    emu->m_frame_done = false;
    emu->m_scanline = 0;
    emu->m_sample_count = 0;
    SDL_ClearAudioStream(emu->m_stream);

    while (SDL_GetAudioStreamQueued(emu->m_stream) > 0)
        ;

    emu->m_bus.cpu.halt_on_brk = false;
    emu->m_bus.ppu.colors = g_colors;
    emu->m_bus.ppu.scanline_cb = &Emulator::render_scanline;
    emu->m_bus.ppu.user_data = emu;
}

void Emulator::sample_callback(Apu *self, ApuSample smp) {
    Emulator *emu = (Emulator *) self->user_data;

    emu->m_audio_buf[emu->m_sample_count] = smp;

    if (++emu->m_sample_count >= AUDIO_BUF_SIZE) {
        SDL_PutAudioStreamData(
            emu->m_stream, emu->m_audio_buf, AUDIO_BUF_SIZE * sizeof(ApuSample)
        );

        emu->m_sample_count = 0;
    }
}
