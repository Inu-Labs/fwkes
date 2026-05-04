#pragma once

#include "../bus.h"
#include "../cpu.h"

struct Vm {
    Vm();

    Bus bus;
    Cpu cpu;
    uint8_t memory[MEMORY_SIZE];
};
