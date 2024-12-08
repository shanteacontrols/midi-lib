add_executable(libmidi-test
    test.cpp
)

target_link_libraries(libmidi-test
    PRIVATE
    liblibmidi-test-common
    libmidi
)

target_compile_definitions(libmidi-test
    PRIVATE
    TEST
)

target_compile_definitions(libmidi
    PUBLIC
    EMU_EEPROM_PAGE_SIZE=128
)

add_test(
    NAME test_build
    COMMAND
    "${CMAKE_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --config "$<CONFIG>"
    --target libmidi-test
)

set_tests_properties(test_build
    PROPERTIES
    FIXTURES_SETUP
    test_fixture
)

add_test(
    NAME test
    COMMAND $<TARGET_FILE:libmidi-test>
)

set_tests_properties(test
    PROPERTIES
    FIXTURES_REQUIRED
    test_fixture
)