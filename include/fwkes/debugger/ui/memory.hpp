#pragma once

#include "../emu.hpp"
#include "../imgui.hpp"
#include "window.hpp"

namespace ui {
    enum class MemoryMap {
        Cpu,
        Ppu,
    };

    class Memory : public Window {
      public:
        Memory(Emulator &emu);

        Emulator &emu() { return m_emu; }

        const Emulator &emu() const { return m_emu; }

        MemoryMap curr_map() const { return m_curr_map; }

        size_t size() const;

      private:
        void main() override;

        MemoryEditor m_mem_edit;
        Emulator &m_emu;
        MemoryMap m_curr_map = MemoryMap::Cpu;
    };
}
