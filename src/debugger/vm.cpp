#include <fwkes/desktop/vm.hpp>

Vm::Vm() {
    this->bus.memory = this->memory;
    this->cpu.bus = &this->bus;

    cpu_init(&this->cpu, nullptr);
}
