#include <fwkes/debugger/ui/registers.hpp>

#include <fonts/codicons.h>

#include <cinttypes>

using namespace ui;

static void reg_u8_field(const char *name, uint8_t *in, uint8_t *out) {
    if (ImGui::InputScalar(
            name, ImGuiDataType_U8, in, nullptr, nullptr, "%02" PRIx8, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        *out = *in;
    }
}

static void reg_u16_field(const char *name, uint16_t *in, uint16_t *out) {
    if (ImGui::InputScalar(
            name, ImGuiDataType_U16, in, nullptr, nullptr, "%04" PRIx16, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        *out = *in;
    }
}

static void status_flag(int n, const char *name, uint8_t flag, uint8_t *p) {
    bool set = (bool) (*p & flag);
    ImGui::Text("%s = %d", name, set);
    ImGui::SameLine();

    ImGui::PushID(n);
    if (ImGui::SmallButton(set ? "0" : "1")) {
        if (set) {
            *p &= ~flag;
        } else {
            *p |= flag;
        }
    }
    ImGui::PopID();
}

Registers::Registers(Emulator &emu)
    : Window{"Register View", ImGuiWindowFlags_AlwaysAutoResize}, m_emu{emu} {}

void Registers::main() {
    section_cpu();
    section_ppu();
}

void Registers::section_cpu() {
    Cpu &cpu = m_emu.bus().cpu;
    uint8_t A = cpu.A;
    uint8_t X = cpu.X;
    uint8_t Y = cpu.Y;
    uint16_t PC = cpu.PC;
    uint8_t S = cpu.S;
    uint8_t P = cpu.P;

    ImGui::SeparatorText("CPU");

    ImGui::PushItemWidth(ImGui::GetFontSize() * 4);

    reg_u8_field("A ", &A, &cpu.A);
    ImGui::SameLine();
    reg_u8_field("X", &X, &cpu.X);
    ImGui::SameLine();
    reg_u8_field("Y", &Y, &cpu.Y);
    reg_u16_field("PC", &PC, &cpu.PC);
    ImGui::SameLine();
    reg_u8_field("S", &S, &cpu.S);
    ImGui::SameLine();
    reg_u8_field("P", &P, &cpu.P);

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static const std::array<std::pair<const char *, uint8_t>, 7> flags = {
        std::make_pair("CARRY", CPU_FLAG_C), std::make_pair("ZERO", CPU_FLAG_Z),
        std::make_pair("INTD", CPU_FLAG_I),  std::make_pair("DEC", CPU_FLAG_D),
        std::make_pair("BREAK", CPU_FLAG_B), std::make_pair("OVER", CPU_FLAG_V),
        std::make_pair("NEG", CPU_FLAG_N)
    };

    for (size_t i = 0; i < flags.size(); ++i) {
        status_flag((int) i, flags[i].first, flags[i].second, &cpu.P);

        if (i == 0 || (i % 3 != 0 && i < flags.size() - 1)) {
            ImGui::SameLine();
        }
    }
}

void Registers::section_ppu() {
    Ppu &ppu = m_emu.bus().ppu;

    ImGui::SeparatorText("PPU");

    uint16_t v = ppu.v;
    uint16_t t = ppu.t;
    uint8_t x = ppu.x;

    ImGui::PushItemWidth(ImGui::GetFontSize() * 3);

    reg_u16_field("v", &v, &ppu.v);
    ImGui::SameLine();
    reg_u16_field("t", &t, &ppu.t);
    ImGui::SameLine();
    reg_u8_field("x", &x, &ppu.x);
    ImGui::SameLine();

    bool w = ppu.w;
    ImGui::Checkbox("w", &w);
    ppu.w = w;

    ImGui::PopItemWidth();

    static const std::array<std::tuple<const char *, uint8_t *, uint8_t>, 14>
        flag_fields = {
            std::make_tuple("Sprite 0 hit", &ppu.status, PPU_STAT_ZERO_HIT),
            std::make_tuple(
                "Sprite 0 overflow", &ppu.status, PPU_STAT_OVERFLOW
            ),
            std::make_tuple("Vblank", &ppu.status, PPU_STAT_VBLANK),
            std::make_tuple("Vblank NMI enable", &ppu.ctrl, PPU_CTRL_NMI_ON),
            std::make_tuple(
                "Increment VRAM by 32", &ppu.ctrl, PPU_CTRL_INC_MODE
            ),
            std::make_tuple("BG at $1000", &ppu.ctrl, PPU_CTRL_BG_TILE_SELECT),
            std::make_tuple("8x16 sprites", &ppu.ctrl, PPU_CTRL_SPR_HEIGHT),
            std::make_tuple(
                "Sprites at $1000", &ppu.ctrl, PPU_CTRL_SPR_TILE_SELECT
            ),
            std::make_tuple("BG enabled", &ppu.mask, PPU_MASK_BG_ON),
            std::make_tuple("Greyscale", &ppu.mask, PPU_MASK_GREYSCALE),
            std::make_tuple("Sprites enabled", &ppu.mask, PPU_MASK_SPR_ON),
            std::make_tuple("Emphasize red", &ppu.mask, PPU_MASK_EMPH_RED),
            std::make_tuple("Emphasize green", &ppu.mask, PPU_MASK_EMPH_GREEN),
            std::make_tuple("Emphasize blue", &ppu.mask, PPU_MASK_EMPH_BLUE),
        };

    if (ImGui::BeginTable("PPUCTRL", 2)) {
        for (size_t i = 0; i < flag_fields.size(); ++i) {
            if (i % 2 == 0) {
                ImGui::TableNextRow();
            }

            ImGui::TableSetColumnIndex(i % 2);

            const char *label = std::get<0>(flag_fields[i]);
            uint8_t *flags = std::get<1>(flag_fields[i]);
            uint8_t flag = std::get<2>(flag_fields[i]);

            unsigned flags32 = *flags;
            ImGui::CheckboxFlags(label, &flags32, flag);
            *flags = flags32 & 0xff;
        }

        ImGui::EndTable();
    }
}
