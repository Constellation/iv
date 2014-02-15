# lv5 test

link_directories("${PROJECT_SOURCE_DIR}/third_party/google-test")
include_directories("${PROJECT_SOURCE_DIR}/third_party/google-test")

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions("-D__STDC_FORMAT_MACROS")
endif()

add_executable(lv5_unit_tests
    test_fpu.cc
    test_jsval.cc
    test_radio_arena.cc
    test_radio_core.cc
    test_suite.cc
    )

target_link_libraries(lv5_unit_tests google-test ${LV5_LIBRARIES})
