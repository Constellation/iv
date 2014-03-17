# iv test

link_directories("${PROJECT_SOURCE_DIR}/third_party/google-test")
include_directories("${PROJECT_SOURCE_DIR}/third_party/google-test")

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions(
        "-Wno-variadic-macros"
        "-fno-strict-aliasing"
        )
endif()

setup_compiler_definitions()

add_executable(iv_unit_tests
    dtoa_fixed.cc
    dtoa_precision.cc
    dtoa_shortest.cc
    test_aero_compiler.cc
    test_aero_exec.cc
    test_aero_filter.cc
    test_aero_incomplete.cc
    test_aero_jit.cc
    test_aero_parser.cc
    test_aero_simple.cc
    test_aero_source_escape.cc
    test_aero_vm.cc
    test_any.cc
    test_arith.cc
    test_assoc_vector.cc
    test_ast_pre_order_visitor.cc
    test_bit_vector.cc
    test_bloom_filter.cc
    test_character.cc
    test_cmdline.cc
    test_conversions.cc
    test_digit_iterator.cc
    test_dtoa.cc
    test_fixed_container.cc
    test_fixed_string_builder.cc
    test_i18n.cc
    test_i18n_number_format.cc
    test_intrusive_list.cc
    test_intrusive_ptr.cc
    test_math.cc
    test_mt19937.cc
    test_os_allocator.cc
    test_parser.cc
    test_ref_counted.cc
    test_segmented_vector.cc
    test_singleton.cc
    test_sip_hash.cc
    test_sorted_vector.cc
    test_space.cc
    test_string_builder.cc
    test_string_view.cc
    test_thread.cc
    test_thread_safe_ref_counted.cc
    test_unicode.cc
    )

target_link_libraries(iv_unit_tests google-test ${IV_LIBRARIES})
add_subdirectory(benchmarks)
