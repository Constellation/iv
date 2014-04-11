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

find_package(LLVM 3.3 REQUIRED all ${EXTRA_LLVM_MODULES})
math(EXPR LV5_LLVM_VER ${LLVM_VERSION_MAJOR}*100+${LLVM_VERSION_MINOR})

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
    ${LLVM_LIBRARY_DIRS}
    )
link_directories_if_exists(/usr/local/lib)
link_directories_if_exists(/opt/local/lib)

include_directories(
    third_party/gc
    ${LLVM_INCLUDE_DIRS}
    )

add_definitions("-DGC_NOT_DLL")

if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    add_definitions(
        "-fno-strict-aliasing"
        "-D__STDC_FORMAT_MACROS"
        )
endif()

# liblv5

string(REPLACE "-Werror" "" LLVM_CXXFLAGS ${LLVM_CXXFLAGS})

add_library(liblv5 STATIC
    context.cc
    global_symbols.cc
    global_data.cc
    jsstring.cc
    breaker/compiler_arithmetic.cc
    breaker/compiler_bitwise.cc
    breaker/compiler_call.cc
    breaker/compiler_comparison.cc
    breaker/compiler_equals.cc
    breaker/stub.cc
    diagram/compiler.cc
    diagram/diagram.cc
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
set_target_properties(
  liblv5 PROPERTIES
  PREFIX ""
  COMPILE_FLAGS "${LLVM_CXXFLAGS}")
mz_merge_static_libraries(liblv5 ALL gc-lib v8_dtoa netlib_dtoa)
add_dependencies(liblv5 gc-lib v8_dtoa netlib_dtoa)
# LLVM libraries should come after "-lld". To ensure that,
# we set LLVM_LIBRARIES flag between LLVM_LDFLAGS.
set(LV5_LIBRARIES liblv5 ${IV_LIBRARIES} "${LLVM_LDFLAGS}" ${LLVM_LIBRARIES} "${LLVM_LDFLAGS}")

# lv5 shell

add_executable(lv5 main.cc)
set_target_properties(
  lv5 PROPERTIES
  COMPILE_FLAGS "${LLVM_CXXFLAGS}")

target_link_libraries(lv5 ${LV5_LIBRARIES})

# build & link backward to lv5 shell
set(BACKWARD_LIBRARIES)
set(BACKWARD_DEFINITIONS)
function(build_and_link_backward)
    if (NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
        set(STACK_DETAILS_DW FALSE)
        find_library(LIB_DW dw)
        if (NOT ("${LIB_DW}" STREQUAL "LIB_DW-NOTFOUND"))
            set(STACK_DETAILS_DW TRUE)
            LIST(APPEND BACKWARD_LIBRARIES "${LIB_DW}")
        endif()
        set(STACK_DETAILS_BFD FALSE)
        find_library(LIB_BFD bfd)
        if (NOT ("${LIB_BFD}" STREQUAL "LIB_BFD-NOTFOUND"))
            set(STACK_DETAILS_BFD TRUE)
            LIST(APPEND BACKWARD_LIBRARIES "${LIB_BFD}")
        endif()

        ###############################################################################
        # COMPILER FLAGS
        ###############################################################################

        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors")
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
        endif()

        ###############################################################################
        # CONFIGS
        ###############################################################################

        set(STACK_DETAILS_BACKTRACE_SYMBOL TRUE CACHE BOOL "Use backtrace symbol")
        set(STACK_DETAILS_DW FALSE CACHE BOOL "Use libdw")
        set(STACK_DETAILS_BFD FALSE CACHE BOOL "Use libbfd")

        set(STACK_WALKING_UNWIND TRUE CACHE BOOL "Use unwind for stack walking")
        set(STACK_WALKING_BACKTRACE FALSE CACHE BOOL "Use backtrace for stack walking")

        macro(map_definitions var_prefix define_prefix)
          foreach(def ${ARGV})
            if (${${var_prefix}${def}})
              LIST(APPEND BACKWARD_DEFINITIONS "${define_prefix}${def}=1")
            endif()
          endforeach()
        endmacro()

        map_definitions("STACK_DETAILS_" "BACKWARD_HAS_" BACKTRACE_SYMBOL DW BFD)
        map_definitions("STACK_WALKING_" "BACKWARD_HAS_" UNWIND BACKTRACE)

        foreach(def ${BACKWARD_DEFINITIONS})
          message(STATUS "${def}")
        endforeach()
        add_library(backward STATIC third_party/backward/backward.cpp)
        target_link_libraries(lv5 backward ${BACKWARD_LIBRARIES})
    endif()
endfunction(build_and_link_backward)
build_and_link_backward()

add_subdirectory(test)
