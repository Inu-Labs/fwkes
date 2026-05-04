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

set(COMPILE_OPTIONS
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -fstack-clash-protection $<$<CONFIG:Debug>:-O0>
)

function(add_unit_test NAME)
    add_executable(${NAME}_test $<TARGET_OBJECTS:fwkes-obj> ${NAME}.c)
    target_compile_options(${NAME}_test PRIVATE ${COMPILE_OPTIONS})
    target_include_directories(${NAME}_test PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(${NAME}_test PRIVATE unity)
endfunction()

# FIXME: refactor tests code so we can enable them again

# add_unit_test(bus)
# add_unit_test(cpu)

# if (BUILD_VISUAL_TESTS)
    # add_subdirectory(6502_snake)
    # add_subdirectory(ppu-pattern-test)
# endif()

# add_subdirectory(nestest)
# add_subdirectory(instr_test-v5)
