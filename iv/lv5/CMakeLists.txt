# lv5 engine

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

set(LV5_SOURCES context.cc)
set(LV5_SOURCES ${LV5_SOURCES} PARENT_SCOPE)

add_executable(lv5 main.cc ${LV5_SOURCES})

target_link_libraries(lv5
    netlib_dtoa
    v8_dtoa
    )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    find_library(LIB_CXX c++)
    target_link_libraries(lv5 ${LIB_CXX})
endif()

find_library(LIB_GC gc)
target_link_libraries(lv5 ${LIB_GC})

find_library(LIB_M m)
target_link_libraries(lv5 ${LIB_M})

find_library(LIB_PTHREAD pthread)
target_link_libraries(lv5 ${LIB_PTHREAD})
