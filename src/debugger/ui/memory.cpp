#include <fwkes/desktop/ui/memory.hpp>

using namespace ui;

Memory::Memory(Vm &vm) : Window{"Memory", 0}, m_vm{vm} {}

void Memory::main() { m_mem_edit.DrawContents(m_vm.memory, MEMORY_SIZE); }
