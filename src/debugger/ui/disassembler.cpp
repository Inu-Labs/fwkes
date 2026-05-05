#include <fwkes/debugger/ui/disassembler.hpp>

#include <fwkes/cpu.h>

#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define U24_COMBINE(lsb, midb, msb)                                            \
    ((uint32_t) ((uint32_t) ((msb) << 16) | (uint32_t) ((midb) << 8) |         \
                 (uint32_t) (lsb)))

using namespace ui;

Disassembler::Disassembler(Emulator &emu)
    : Window{"Disassembler", ImGuiWindowFlags_None}, m_emu{emu} {
    m_entries.reserve(0x8000);
    strcpy(m_buf, "<unknown>");

    update();
}

void Disassembler::update() {
    m_entries.clear();

    if (!m_emu.loaded()) {
        m_entries.emplace_back(
            0, 0, "No ROM is loaded. Click File->Load ROM and pick a ROM!"
        );
        return;
    }

    const Disk &disk = m_emu.bus().disk;
    const uint8_t *prg = disk.prg;
    size_t prg_size = disk.prg_size;

    for (size_t i = 0, instr_size = 0; i < prg_size; i += instr_size) {
        const char *disas = disassemble(&prg[i], i, instr_size);
        uint32_t data = 0;

        assert(instr_size >= 1 && instr_size <= 3);

        if (instr_size == 1) {
            data = U24_COMBINE(prg[i], 0, 0);
        } else if (instr_size == 2) {
            data = U24_COMBINE(prg[i], prg[i + 1], 0);
        } else if (instr_size == 3) {
            data = U24_COMBINE(prg[i], prg[i + 1], prg[i + 2]);
        }

        m_entries.emplace_back((uint16_t) i, data, disas);
    }
}

const char *Disassembler::disassemble(
    const uint8_t *opcode, size_t pc, size_t &instr_size
) {
    instr_size = cpu_disassemble(opcode, (uint16_t) pc, m_buf);

    return m_buf;
}

void Disassembler::main() {
    if (!ImGui::BeginTable(
            "Memory Content", 3,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit
        )) {
        ImGui::EndTable();
        ImGui::End();

        return;
    }

    ImGui::TableSetupColumn("Offset");
    ImGui::TableSetupColumn("Data (LE)");
    ImGui::TableSetupColumn("Disassembled");
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin((int) m_entries.size());

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const Entry &entry = m_entries[(size_t) row];

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            uint16_t offset = std::get<0>(entry);
            ImGui::Text("%04" PRIx16, offset);

            ImGui::TableSetColumnIndex(1);
            uint32_t data = std::get<1>(entry);
            ImGui::Text(
                "%02" PRIx8 "%02" PRIx8 "%02" PRIx8, (data & 0x000000ff),
                (data & 0x0000ff00) >> 8, (data & 0x00ff0000) >> 16
            );

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(std::get<2>(entry).c_str());

            if (row == m_emu.bus().cpu.PC) {
                ImU32 color = ImGui::GetColorU32(ImVec4(1.0, 1.0, 0.0, 0.4f));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, color);
            }
        }
    }

    ImGui::EndTable();
}
