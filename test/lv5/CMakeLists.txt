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

target_link_libraries(lv5_unit_tests gtest liblv5)
link_lv5(lv5_unit_tests)
