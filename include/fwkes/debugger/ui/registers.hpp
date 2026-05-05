#pragma once

#include "../emu.hpp"
#include "window.hpp"

namespace ui {
    class Registers : public Window {
    public:
        Registers(Emulator &emu);

    private:
        void main() override;

        void section_cpu();
        void section_ppu();

        Emulator &m_emu;
    };
}
