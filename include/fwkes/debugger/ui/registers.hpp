#pragma once

#include "../vm.hpp"
#include "window.hpp"

namespace ui {
    class Registers : public Window {
    public:
        Registers(Vm &vm);

    private:
        void main() override;

        Vm &m_vm;
    };
}
