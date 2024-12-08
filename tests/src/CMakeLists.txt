find_package(glog 0.6.0 REQUIRED)

add_library(liblibmidi-test-common)

target_sources(liblibmidi-test-common
    PRIVATE
    main.cpp
)

target_include_directories(liblibmidi-test-common
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(liblibmidi-test-common
    PUBLIC
    glog::glog
    gtest
)

add_subdirectory(ble)