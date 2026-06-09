#include <fwkes/debugger/ui/playback_recorder.hpp>

#include <fwkes/bus.h>
#include <fwkes/joyplayer.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <imgui.h>

using namespace ui;

static std::string format_buttons(uint8_t buttons) {
    int a = (bool) (buttons & 0x80);
    int b = (bool) (buttons & 0x40);
    int select = (bool) (buttons & 0x20);
    int start = (bool) (buttons & 0x10);
    int up = (bool) (buttons & 0x08);
    int down = (bool) (buttons & 0x04);
    int left = (bool) (buttons & 0x02);
    int right = (bool) (buttons & 0x01);

    return std::format(
        "A = {} | B = {} | SELECT = {} | START = {} | UP = {} | DOWN = {} | "
        "LEFT = {} | RIGHT = {}",
        a, b, select, start, up, down, left, right
    );
}

PlaybackRecorder::PlaybackRecorder(Emulator &emu)
    : Window{"Playback Recorder", 0}, m_emu{emu} {}

void PlaybackRecorder::update() {
    if (m_entries.size() > 1000) {
        m_entries.clear();
    }

    const JoyPlayer &jp = m_emu.bus().joyplayer;

    if (!m_emu.loaded() || !jp.active) {
        return;
    }

    uint8_t new_buttons = jp.current_buttons;

    if (new_buttons != m_emu.old_joyplayer_state()) {
        m_entries.emplace_back(new_buttons, m_emu.joyplayer_timestamp());
    }
}

void PlaybackRecorder::main() {
    if (!ImGui::BeginTable(
            "Input Events", 2,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit
        )) {
        ImGui::EndTable();
        ImGui::End();

        return;
    }

    ImGui::TableSetupColumn("Buttons");
    ImGui::TableSetupColumn("Timestamp");
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin((int) m_entries.size());

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const Entry &entry = m_entries[(size_t) row];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(format_buttons(std::get<0>(entry)).c_str());

            ImGui::TableSetColumnIndex(1);
            CycleCounter ts = std::get<1>(entry);
            ImGui::Text("%" CYCLE_COUNTER_PRIu, ts);
        }
    }

    ImGui::EndTable();
}
