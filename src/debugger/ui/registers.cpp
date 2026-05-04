#include <fwkes/desktop/ui/registers.hpp>

#include <cinttypes>

using namespace ui;

Registers::Registers(Vm &Vm)
    : Window{"Registers", ImGuiWindowFlags_AlwaysAutoResize}, m_vm{Vm} {}

void Registers::main() {
    uint8_t A = m_vm.cpu.A;
    uint8_t X = m_vm.cpu.X;
    uint8_t Y = m_vm.cpu.Y;
    uint16_t PC = m_vm.cpu.PC;
    uint8_t S = m_vm.cpu.S;
    uint8_t P = m_vm.cpu.P;

    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

    if (ImGui::InputScalar(
            "A", ImGuiDataType_U8, &A, nullptr, nullptr, "%02" PRIx16, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.A = A;
    }

    if (ImGui::InputScalar(
            "X", ImGuiDataType_U8, &X, nullptr, nullptr, "%02" PRIx8, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.X = X;
    }

    if (ImGui::InputScalar(
            "Y", ImGuiDataType_U8, &Y, nullptr, nullptr, "%02" PRIx8, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.Y = Y;
    }

    if (ImGui::InputScalar(
            "PC", ImGuiDataType_U16, &PC, nullptr, nullptr, "%04" PRIx16, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.PC = PC;
    }

    if (ImGui::InputScalar(
            "S", ImGuiDataType_U8, &S, nullptr, nullptr, "%02" PRIx8, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.S = X;
    }

    if (ImGui::InputScalar(
            "P", ImGuiDataType_U8, &P, nullptr, nullptr, "%02" PRIx8, 0
        ) &&
        ImGui::IsItemDeactivatedAfterEdit()) {
        m_vm.cpu.P = P;
    }

    ImGui::PopItemWidth();
}
