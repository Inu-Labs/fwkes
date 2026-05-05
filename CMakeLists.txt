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

cmake_minimum_required(VERSION 3.12)

if (BUILD_RP2350)
    set(PICO_COPY_TO_RAM ON CACHE BOOL "")

    if (BUILD_PIMORONI2)
        set(PICO_BOARD "pimoroni_pico_plus2_rp2350" CACHE STRING "")
    else()
        set(PICO_BOARD "pico2" CACHE STRING "")
    endif()

    set(PICO_PLATFORM "rp2350" CACHE STRING "")

    include(cmake/pico_sdk_import.cmake)
    project(fwkes LANGUAGES C CXX ASM VERSION 1.0.0)
else()
    project(fwkes LANGUAGES C CXX VERSION 1.0.0)
endif()


option(BUILD_RP2350 "build for RP2350 based boards" OFF)
option(BUILD_DESKTOP "build desktop backend (PC only)" OFF)
option(BUILD_DEBUGGER "build debugger (PC only)" OFF)
option(BUILD_PIMORONI2 "specify Pimoroni Pico Plus 2 as a target RP2350 board" OFF)
option(BUILD_TESTS "build unit tests" OFF)
option(BUILD_VISUAL_TESTS "build tests with graphics" OFF)
option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BUILD_DEBUG_PLAYBACK "Enable debug playback controls" OFF)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (BUILD_RP2350)
    pico_sdk_init()
endif()

add_subdirectory(third-party)
add_subdirectory(src)

if (BUILD_TESTS OR BUILD_VISUAL_TESTS)
    enable_testing()

    add_subdirectory(third-party/Unity)
    add_subdirectory(tests)
endif()
