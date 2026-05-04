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

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/fwkes/rp2350")

add_executable(fwkes-rp2350
    audio.c
    main.c
    psram.c
    video.c
    joypads.c
    "${INCLUDE_DIR}/audio.h"
    "${INCLUDE_DIR}/psram.h"
    "${INCLUDE_DIR}/video.h"
    "${INCLUDE_DIR}/joypads.h"
)

pico_enable_stdio_uart(fwkes-rp2350 0)
pico_enable_stdio_usb(fwkes-rp2350 1)
pico_generate_pio_header(fwkes-rp2350 "${CMAKE_CURRENT_LIST_DIR}/pio/joypad_read.pio")

target_compile_definitions(fwkes-rp2350 PRIVATE -DBUILD_RP2350)

target_link_libraries(fwkes-rp2350 PRIVATE
    pico_stdlib
    pico_multicore
    pico_util
    hardware_gpio
    hardware_pwm
    hardware_spi
    hardware_pio
    fwkes-obj
    ff
    libdvi
    lwmem
)

set(COMPILE_OPTIONS
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -O3
)

# set_property(TARGET fwkes-rp2350 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

target_compile_options(fwkes-rp2350 PRIVATE ${COMPILE_OPTIONS})
target_include_directories(fwkes-rp2350 PRIVATE "${PROJECT_SOURCE_DIR}/include")

pico_add_extra_outputs(fwkes-rp2350)
