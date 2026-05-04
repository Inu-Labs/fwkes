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

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/fwkes")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(HEADERS
    "${INCLUDE_DIR}/apu.h"
    "${INCLUDE_DIR}/bits.h"
    "${INCLUDE_DIR}/bus.h"
    "${INCLUDE_DIR}/joypad.h"
    "${INCLUDE_DIR}/cpu.h"
    "${INCLUDE_DIR}/disk.h"
    "${INCLUDE_DIR}/fs.h"
    "${INCLUDE_DIR}/fwx/common.h"
    "${INCLUDE_DIR}/fwx/private.h"
    "${INCLUDE_DIR}/fwx/public.h"
    "${INCLUDE_DIR}/mappers/shared.h"
    "${INCLUDE_DIR}/mappers/map000.h"
    "${INCLUDE_DIR}/mappers/map002.h"
    "${INCLUDE_DIR}/mappers/map004.h"
    "${INCLUDE_DIR}/log.h"
    "${INCLUDE_DIR}/ppu.h"
    "${INCLUDE_DIR}/trace.h"
)

set(SOURCES
    "${SRC_DIR}/apu.c"
    "${SRC_DIR}/bus.c"
    "${SRC_DIR}/joypad.c"
    "${SRC_DIR}/cpu.c"
    "${SRC_DIR}/disk.c"
    "${SRC_DIR}/fs.c"
    "${SRC_DIR}/fwx.c"
    "${SRC_DIR}/log.c"
    "${SRC_DIR}/ppu.c"
    "${SRC_DIR}/trace.c"
    "${SRC_DIR}/mappers/map000.c"
    "${SRC_DIR}/mappers/map002.c"
    "${SRC_DIR}/mappers/map004.c"
)

add_library(fwkes-obj OBJECT ${HEADERS} ${SOURCES})

set(COMPILE_OPTIONS
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow $<$<NOT:$<BOOL:${BUILD_RP2350}>>:-fstack-clash-protection> $<$<CONFIG:Debug>:-O0>
)

if (USE_ASAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=address)
    target_link_options(fwkes-obj PRIVATE -fsanitize=address)
elseif (USE_UBSAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=undefined)
    target_link_options(fwkes-obj PRIVATE -fsanitize=undefined)
endif()

target_compile_options(fwkes-obj PRIVATE ${COMPILE_OPTIONS})
target_include_directories(fwkes-obj PRIVATE "${PROJECT_SOURCE_DIR}/include")

if (BUILD_TESTS)
    add_library(fwkes-nestest-obj OBJECT ${HEADERS} ${SOURCES})

set(COMPILE_OPTIONS
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow $<$<NOT:$<BOOL:${BUILD_RP2350}>>:-fstack-clash-protection> $<$<CONFIG:Debug>:-O0>
)

    target_compile_options(fwkes-nestest-obj PRIVATE ${COMPILE_OPTIONS})
    target_compile_definitions(fwkes-nestest-obj PRIVATE -DNESTEST -DTRACE_ON -DTRACE_BUS_ON -DTRACE_CPU_ON -DTRACE_PPU_ON -DTRACE_FWX_ON)
    target_include_directories(fwkes-nestest-obj PRIVATE "${PROJECT_SOURCE_DIR}/include")
endif()

if (BUILD_DESKTOP)
    add_subdirectory(desktop)
endif()

if(BUILD_RP2350)
    target_compile_definitions(fwkes-obj PRIVATE -DBUILD_RP2350)
    target_link_libraries(fwkes-obj PRIVATE
        ff
        lwmem
        pico_stdlib_headers
        hardware_pio_headers
    )

    add_subdirectory(rp2350)
endif()
