# liblv5 & lv5

enable_language(ASM)

# prelude
set(CMAKE_OSX_ARCHITECTURES "x86_64")
add_subdirectory(third_party/gc)


setup_compiler_definitions()
add_subdirectory(third_party/netlib_dtoa)
add_subdirectory(third_party/v8_dtoa)

link_directories(
    third_party/netlib_dtoa
    third_party/v8_dtoa
    )

# link local directories
link_directories_if_exists(/usr/local/lib)
link_directories_if_exists(/opt/local/lib)

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions(
        "-fno-strict-aliasing"
        "-D__STDC_FORMAT_MACROS"
        )
endif()

# liblv5

set(ACCELERATOR_SOURCE)

if (accelerator)
  set(ACCELERATOR_SOURCE
      accelerator/vm.x64.S
      )
endif()

set(LIBLV5_SOURCE
    ${ACCELERATOR_SOURCE}
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

add_library(liblv5 STATIC ${LIBLV5_SOURCE})
set_target_properties(liblv5 PROPERTIES PREFIX "")

target_link_libraries(liblv5 netlib_dtoa v8_dtoa)

# lv5 shell

add_executable(lv5 main.cc)

target_link_libraries(lv5 liblv5)

# link settings

macro(link_iv target)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(LIB_CXX "")
        find_library(LIB_CXX c++)
        if (NOT "${LIB_CXX}" STREQUAL "NOTFOUND")
            target_link_libraries(${target} ${LIB_CXX})
        endif()
    endif()

    if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
        find_library(LIB_M m)
        target_link_libraries(${target} ${LIB_M})

        find_library(LIB_PTHREAD pthread)
        target_link_libraries(${target} ${LIB_PTHREAD})
    endif()
endmacro(link_iv)

macro(link_lv5 target)
    link_iv(${target})
    target_link_libraries(${target} gc-lib)
endmacro(link_lv5)

link_lv5(lv5)
