set(DEBUGGER_HEADERS
    "${INCLUDE_DIR}/debugger/ui/canvas.hpp"
    "${INCLUDE_DIR}/debugger/ui/disassembler.hpp"
    "${INCLUDE_DIR}/debugger/ui/memory.hpp"
    "${INCLUDE_DIR}/debugger/ui/registers.hpp"
    "${INCLUDE_DIR}/debugger/ui/playback_recorder.hpp"
    "${INCLUDE_DIR}/debugger/ui/ui.hpp"
    "${INCLUDE_DIR}/debugger/ui/window.hpp"
    "${INCLUDE_DIR}/debugger/emu.hpp"
    "${INCLUDE_DIR}/debugger/imgui.hpp"
    "${INCLUDE_DIR}/debugger/main.hpp"
    "${INCLUDE_DIR}/debugger/message.hpp"
)

set(DEBUGGER_SOURCES
    "${SRC_DIR}/debugger/ui/canvas.cpp"
    "${SRC_DIR}/debugger/ui/disassembler.cpp"
    "${SRC_DIR}/debugger/ui/memory.cpp"
    "${SRC_DIR}/debugger/ui/registers.cpp"
    "${SRC_DIR}/debugger/ui/playback_recorder.cpp"
    "${SRC_DIR}/debugger/ui/ui.cpp"
    "${SRC_DIR}/debugger/ui/window.cpp"
    "${SRC_DIR}/debugger/emu.cpp"
    "${SRC_DIR}/debugger/main.cpp"
    "${SRC_DIR}/debugger/imgui.cpp"
)

add_executable(fwkes-debugger $<TARGET_OBJECTS:fwkes-obj> ${DEBUGGER_HEADERS} ${DEBUGGER_SOURCES})
target_compile_options(fwkes-debugger PRIVATE ${COMPILE_OPTIONS})
target_compile_definitions(fwkes-debugger PRIVATE -DNESTEST -DTRACE_ON -DTRACE_BUS_ON -DTRACE_CPU_ON -DTRACE_PPU_ON -DTRACE_FWX_ON)
target_include_directories(fwkes-debugger PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(fwkes-debugger PRIVATE SDL3 m imgui ImGuiFileDialog imgui_memory_editor)

if (USE_ASAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=address)
    target_link_options(fwkes-debugger PRIVATE -fsanitize=address)
elseif (USE_UBSAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=undefined)
    target_link_options(fwkes-debugger PRIVATE -fsanitize=undefined)
endif()
