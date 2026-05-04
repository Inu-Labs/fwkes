set(SOURCES
    "diskio.c"
    "diskio.h"
    "ff.c"
    "ff.h"
    "ffconf.h"
    "ffsystem.c"
    "ffunicode.c"
    "platform.c"
    "platform.h"
    "storage.c"
    "storage.h"
)

add_library(ff STATIC ${SOURCES})
target_include_directories(ff PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ff PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_spi
)

