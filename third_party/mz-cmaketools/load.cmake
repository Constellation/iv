cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(MZ_TOOLS_PATH "${CMAKE_CURRENT_LIST_DIR}")

macro(mz_message MSG)
    message("-- ${MSG}")
endmacro()

#set(MZ_MSG_DEBUG TRUE)

macro(mz_debug_message MSG)
    if(MZ_MSG_DEBUG)
        mz_message(${MSG})
    endif()
endmacro()

macro(mz_error_message MSG)
    message(SEND_ERROR "!! ${MSG}")
    return()
endmacro()

# user info
message("-- configuring for build type: ${CMAKE_BUILD_TYPE}")
