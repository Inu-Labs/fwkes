#include <fwkes/debugger/ui/memory.hpp>

#include <fwkes/bus.h>

using namespace ui;

static ImU8 read_byte(const ImU8 *mem, size_t off, void *user_data) {
    (void) mem;

    const Memory *mem_view = (const Memory *) user_data;
    const Bus &bus = mem_view->emu().bus();
    uint16_t addr = (uint16_t) off;

    switch (mem_view->curr_map()) {
    case MemoryMap::Cpu:
        return bus_peek(&bus, addr);
    case MemoryMap::Ppu:
        return ppu_peek(&bus.ppu, addr);
    }
}

static void write_byte(ImU8 *mem, size_t off, ImU8 d, void *user_data) {
    (void) mem;
    Memory *mem_view = (Memory *) user_data;
    Bus &bus = mem_view->emu().bus();
    uint16_t addr = (uint16_t) off;

    switch (mem_view->curr_map()) {
    case MemoryMap::Cpu:
        bus_write(&bus, addr, d);

        break;
    case MemoryMap::Ppu:
        ppu_write(&bus.ppu, addr, d);

        break;
    }
}

Memory::Memory(Emulator &emu) : Window{"Memory View", 0}, m_emu{emu} {
    m_mem_edit.ReadFn = read_byte;
    m_mem_edit.WriteFn = write_byte;
    m_mem_edit.UserData = this;
}

void Memory::main() {
    static const char *mem_maps[] = {"CPU", "PPU"};

    ImGui::BeginDisabled(!m_emu.loaded());

    ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
    ImGui::Combo("Memory Map", (int *) &m_curr_map, mem_maps, 2);
    ImGui::PopItemWidth();
    m_mem_edit.DrawContents(nullptr, m_emu.loaded() ? size() : 0);

    ImGui::EndDisabled();
}

size_t Memory::size() const {
    switch (m_curr_map) {
    case MemoryMap::Cpu:
        return 0x10000;
    case MemoryMap::Ppu:
        return 0x4000;
    }
}
