cmake_minimum_required(VERSION 3.22)

project(libmidi)

include(CTest)

add_library(libmidi STATIC)

target_sources(libmidi
    PRIVATE
    src/midi.cpp
    src/transport/ble.cpp
    src/transport/serial.cpp
    src/transport/usb.cpp
)

target_include_directories(libmidi
    PUBLIC
    include
)

add_custom_target(libmidi-format
    COMMAND echo Checking code formatting...
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/code_format.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(libmidi-format PROPERTIES EXCLUDE_FROM_ALL TRUE)

if (CMAKE_EXPORT_COMPILE_COMMANDS STREQUAL ON)
    add_custom_target(libmidi-lint
        COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/clang_tidy_wrapper.sh --output=${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-fixes.yml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_target_properties(libmidi-lint PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_dependencies(libmidi-lint libmidi)
endif()

if (BUILD_TESTING_MIDI STREQUAL ON)
    add_subdirectory(tests)
endif()