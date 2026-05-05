#pragma once

#include "../emu.hpp"
#include "window.hpp"

#include <string>
#include <tuple>
#include <vector>

namespace ui {
    class Disassembler : public Window {
      public:
        Disassembler(Emulator &emu);

        void update();

      private:
        using Entry = std::tuple<uint16_t, uint32_t, std::string>;

        const char *disassemble(const uint8_t *opcode, size_t pc, size_t &instr_size);
        void main() override;

        std::vector<Entry> m_entries;
        char m_buf[CPU_DISAS_BUF_SIZE] = {};
        Emulator &m_emu;
    };
}
