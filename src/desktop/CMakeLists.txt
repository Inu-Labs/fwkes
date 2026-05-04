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

add_executable(fwkes-desktop $<TARGET_OBJECTS:fwkes-obj> main.c)
target_compile_options(fwkes-desktop PRIVATE ${COMPILE_OPTIONS})
target_compile_definitions(fwkes-desktop PRIVATE -DNESTEST -DTRACE_ON -DTRACE_BUS_ON -DTRACE_CPU_ON -DTRACE_PPU_ON -DTRACE_FWX_ON)
target_include_directories(fwkes-desktop PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(fwkes-desktop PRIVATE SDL3 m)

if (USE_ASAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=address)
    target_link_options(fwkes-desktop PRIVATE -fsanitize=address)
elseif (USE_UBSAN)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -fsanitize=undefined)
    target_link_options(fwkes-desktop PRIVATE -fsanitize=undefined)
endif()
