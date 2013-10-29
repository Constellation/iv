# liblv5 & lv5

# prelude
add_subdirectory(third_party/netlib_dtoa)
add_subdirectory(third_party/v8_dtoa)

link_directories(
    third_party/netlib_dtoa
    third_party/v8_dtoa
    )

# link local directories
if(EXISTS "/usr/local/lib/")
    link_directories(/usr/local/lib)
endif()
if(EXISTS "/opt/local/lib/")
    link_directories(/opt/local/lib)
endif()

add_definitions(
    "-fno-strict-aliasing"
    "-D__STDC_FORMAT_MACROS"
    )

# liblv5

add_library(liblv5 STATIC
    context.cc
    runtime/array.cc
    runtime/array_buffer.cc
    runtime/boolean.cc
    runtime/data_view.cc
    runtime/date.cc
    runtime/error.cc
    runtime/function.cc
    runtime/global.cc
    runtime/i18n.cc
    runtime/json.cc
    runtime/map.cc
    runtime/math.cc
    runtime/number.cc
    runtime/object.cc
    runtime/reflect.cc
    runtime/regexp.cc
    runtime/set.cc
    runtime/string.cc
    runtime/symbol.cc
    runtime/weak_map.cc
    )

set_target_properties(liblv5 PROPERTIES PREFIX "")

# lv5 shell

add_executable(lv5 main.cc)

target_link_libraries(lv5
    netlib_dtoa
    v8_dtoa
    liblv5
    )

# link settings

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    find_library(LIB_CXX c++)
    target_link_libraries(liblv5 ${LIB_CXX})
    target_link_libraries(lv5 ${LIB_CXX})
endif()

find_library(LIB_GC gc)
target_link_libraries(liblv5 ${LIB_GC})
target_link_libraries(lv5 ${LIB_GC})

find_library(LIB_M m)
target_link_libraries(liblv5 ${LIB_M})
target_link_libraries(lv5 ${LIB_M})

find_library(LIB_PTHREAD pthread)
target_link_libraries(liblv5 ${LIB_PTHREAD})
target_link_libraries(lv5 ${LIB_PTHREAD})
