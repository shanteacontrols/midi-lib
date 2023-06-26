cmake_minimum_required(VERSION 3.22)

project(MIDI)

include(CTest)

add_library(midi-lib STATIC)

target_sources(midi-lib
    PRIVATE
    src/MIDI.cpp
    src/transport/BLE.cpp
    src/transport/Serial.cpp
    src/transport/USB.cpp
)

target_include_directories(midi-lib
    PUBLIC
    include
)

add_custom_target(midi-format
    COMMAND echo Checking code formatting...
    COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/code_format.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(midi-format PROPERTIES EXCLUDE_FROM_ALL TRUE)

if (CMAKE_EXPORT_COMPILE_COMMANDS STREQUAL ON)
    add_custom_target(midi-lint
        COMMAND ${CMAKE_CURRENT_LIST_DIR}/scripts/clang_tidy_wrapper.sh --output=${CMAKE_CURRENT_BINARY_DIR}/clang-tidy-fixes.yml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_target_properties(midi-lint PROPERTIES EXCLUDE_FROM_ALL TRUE)
    add_dependencies(midi-lint midi-lib)
endif()

if (BUILD_TESTING_MIDI STREQUAL ON)
    add_subdirectory(tests)
endif()