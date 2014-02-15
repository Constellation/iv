# liblv5 & lv5

# prelude
set(CMAKE_OSX_ARCHITECTURES "x86_64")
add_subdirectory(third_party/gc)


setup_compiler_definitions()
add_subdirectory(third_party/netlib_dtoa)
add_subdirectory(third_party/v8_dtoa)

find_package(LLVM 3.3 REQUIRED
   all-targets bitwriter linker ipo instrumentation backend support tablegen asmparser lto ${EXTRA_LLVM_MODULES})
math(EXPR LV5_LLVM_VER ${LLVM_VERSION_MAJOR}*100+${LLVM_VERSION_MINOR})

link_directories(
    third_party/netlib_dtoa
    third_party/v8_dtoa
    ${LLVM_LIBRARY_DIRS}
    )
# link local directories
link_directories_if_exists(/usr/local/lib)
link_directories_if_exists(/opt/local/lib)

include_directories(
    third_party/gc
    ${LLVM_INCLUDE_DIRS}
    )

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
target_link_libraries(liblv5 netlib_dtoa v8_dtoa)
set_target_properties(
  liblv5 PROPERTIES
  COMPILE_FLAGS "${LLVM_CXXFLAGS}")

set(LV5_LIBRARIES liblv5 ${IV_LIBRARIES} gc-lib ${LLVM_LIBRARIES})

# lv5 shell

add_executable(lv5 main.cc)
set_target_properties(
  lv5 PROPERTIES
  COMPILE_FLAGS "${LLVM_CXXFLAGS}")

target_link_libraries(lv5 ${LV5_LIBRARIES})

add_subdirectory(test)
