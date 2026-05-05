#pragma once

#include "../emu.hpp"
#include "window.hpp"

#include <vector>

namespace ui {
    class PlaybackRecorder : public Window {
      public:
        PlaybackRecorder(Emulator &emu);

        void update();

      private:
        using Entry = std::tuple<uint8_t, CycleCounter>;

        void main() override;

        std::vector<Entry> m_entries;
        Emulator &m_emu;
    };
}
