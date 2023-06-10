find_package(glog 0.6.0 REQUIRED)

add_library(midi-test-common)

target_sources(midi-test-common
    PRIVATE
    main.cpp
)

target_include_directories(midi-test-common
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(midi-test-common
    PUBLIC
    glog::glog
    gtest
)

add_subdirectory(ble)