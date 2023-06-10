add_executable(midi-test
    test.cpp
)

target_link_libraries(midi-test
    PRIVATE
    midi-test-common
    midi-lib
)

target_compile_definitions(midi-test
    PRIVATE
    TEST
)

target_compile_definitions(midi-lib
    PUBLIC
    EMU_EEPROM_PAGE_SIZE=128
)

add_test(
    NAME test_build
    COMMAND
    "${CMAKE_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --config "$<CONFIG>"
    --target midi-test
)

set_tests_properties(test_build
    PROPERTIES
    FIXTURES_SETUP
    test_fixture
)

add_test(
    NAME test
    COMMAND $<TARGET_FILE:midi-test>
)

set_tests_properties(test
    PROPERTIES
    FIXTURES_REQUIRED
    test_fixture
)