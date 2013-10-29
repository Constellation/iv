# lv5 test

add_definitions("-D__STDC_FORMAT_MACROS")

link_directories(
    ${PROJECT_SOURCE_DIR}/iv/lv5/third_party/netlib_dtoa
    ${PROJECT_SOURCE_DIR}/iv/lv5/third_party/v8_dtoa
    )

add_executable(lv5_unit_tests
    main.cc
    test_fpu.cc
    test_jsval.cc
    test_radio_arena.cc
    test_radio_core.cc
    test_suite.cc
    )

find_library(LIB_GC gc)
target_link_libraries(lv5_unit_tests ${LIB_GC})

find_library(LIB_M m)
target_link_libraries(lv5_unit_tests ${LIB_M})

find_library(LIB_PTHREAD pthread)
target_link_libraries(lv5_unit_tests ${LIB_PTHREAD})

target_link_libraries(lv5_unit_tests gtest liblv5)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    find_library(LIB_CXX c++)
    target_link_libraries(lv5_unit_tests ${LIB_CXX})
endif()

target_link_libraries(lv5_unit_tests
    netlib_dtoa
    v8_dtoa
    )
