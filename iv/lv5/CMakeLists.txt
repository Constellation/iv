# liblv5 & lv5

# prelude
include(MergeStaticLibraries)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

function(build_gc)
    add_subdirectory(third_party/gc)
endfunction(build_gc)
build_gc()

setup_compiler_definitions()
add_subdirectory(third_party/netlib_dtoa)
add_subdirectory(third_party/v8_dtoa)

# include directories
include_directories(
    third_party
    third_party/gc
    )

# link directories
link_directories(
    third_party/netlib_dtoa
    third_party/v8_dtoa
    third_party/gc
    )
link_directories_if_exists(/usr/local/lib)
link_directories_if_exists(/opt/local/lib)

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions(
        "-fno-strict-aliasing"
        "-D__STDC_FORMAT_MACROS"
        )
endif()

# liblv5

add_library(liblv5 STATIC
    context.cc
    global_symbols.cc
    runtime/array.cc
    runtime/array_buffer.cc
    runtime/array_iterator.cc
    runtime/boolean.cc
    runtime/data_view.cc
    runtime/date.cc
    runtime/error.cc
    runtime/function.cc
    runtime/global.cc
    runtime/i18n.cc
    runtime/json.cc
    runtime/map.cc
    runtime/map_iterator.cc
    runtime/math.cc
    runtime/number.cc
    runtime/object.cc
    runtime/reflect.cc
    runtime/regexp.cc
    runtime/set.cc
    runtime/set_iterator.cc
    runtime/string.cc
    runtime/string_iterator.cc
    runtime/symbol.cc
    runtime/weak_map.cc
    radio/block_control.cc
    radio/core.cc
    )
set_target_properties(liblv5 PROPERTIES PREFIX "")
mz_merge_static_libraries(liblv5 ALL gc-lib v8_dtoa netlib_dtoa)
add_dependencies(liblv5 gc-lib v8_dtoa netlib_dtoa)
# target_link_libraries(liblv5 gc-lib v8_dtoa netlib_dtoa)

set(LV5_LIBRARIES ${IV_LIBRARIES} liblv5)

# lv5 shell

add_executable(lv5 main.cc)
target_link_libraries(lv5 ${LV5_LIBRARIES})

add_subdirectory(test)
