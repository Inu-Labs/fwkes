# Copyright (C) 2025-present InuLabs
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

if (BUILD_DESKTOP OR BUILD_TESTS)
    find_package(SDL3 CONFIG REQUIRED)
endif()

if (BUILD_DESKTOP)
    set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/third-party/imgui")
    set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

    set(IMGUI_HEADERS
        "${IMGUI_DIR}/imconfig.h"
        "${IMGUI_DIR}/imgui.h"
        "${IMGUI_DIR}/imgui_internal.h"
        "${IMGUI_DIR}/imstb_rectpack.h"
        "${IMGUI_DIR}/imstb_textedit.h"
        "${IMGUI_DIR}/imstb_truetype.h"
        "${IMGUI_DIR}/misc/cpp/imgui_stdlib.h"
    )

    set(IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
    )

    set(IMGUI_BACKENDS_HEADERS
        "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.h"
        "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer3.h"
    )

    set(IMGUI_BACKENDS_SOURCES
        "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.cpp"
        "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer3.cpp"
    )

    add_library(
        imgui
        STATIC
        ${IMGUI_HEADERS}
        ${IMGUI_BACKENDS_HEADERS}
        ${IMGUI_SOURCES}
        ${IMGUI_BACKENDS_SOURCES}
    )

    set(IMGUI_CLUB_DIR "${PROJECT_SOURCE_DIR}/third-party/imgui_club")

    add_library(imgui_memory_editor INTERFACE "${IMGUI_CLUB_DIR}/imgui_memory_editor/imgui_memory_editor.h")
    target_include_directories(imgui_memory_editor SYSTEM INTERFACE "${IMGUI_CLUB_DIR}/imgui_memory_editor")

    target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR} ${IMGUI_BACKENDS_DIR})
    target_link_libraries(imgui PRIVATE SDL3::SDL3 ${OPENGL_LIBRARIES})

    add_subdirectory(ImGuiFileDialog)
    target_include_directories(ImGuiFileDialog SYSTEM PUBLIC ${IMGUI_DIR})
endif()

if (BUILD_RP2350)
    add_subdirectory(PicoDVI/software)
    add_subdirectory(ff)
    add_subdirectory(lwmem)
endif()
