#pragma once

#include "window.hpp"
#include "../vm.hpp"

#include <imgui_memory_editor.h>

namespace ui {
    class Memory : public Window {
    public:
        Memory(Vm &vm);

    private:
        void main() override;

        MemoryEditor m_mem_edit;
        Vm &m_vm;
    };
}
