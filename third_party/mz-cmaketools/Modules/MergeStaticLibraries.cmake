#
# This macro creates a true static library bundle with debug and release configurations
# TARGET - the output library, or target, that you wish to contain all of the object files
# CONFIGURATION - DEBUG, RELEASE or ALL
# LIBRARIES - a list of all of the static libraries you want merged into the TARGET
#
# Example use:
# merge_static_libraries (mytarget ALL ${MY_STATIC_LIBRARIES})
#

macro(mz_merge_static_libraries TARGET CONFIGURATION)

    set(_libraries ${ARGN})
    set(LIBRARIES)
    set(LIBRARIES_D)
    foreach(_lib ${_libraries})
        if(TARGET ${_lib})
            get_property(_lib_loc TARGET ${_lib} PROPERTY LOCATION)
            set(LIBRARIES ${LIBRARIES};${_lib_loc})
            get_property(_lib_loc TARGET ${_lib} PROPERTY LOCATION_DEBUG)
            set(LIBRARIES_D ${LIBRARIES_D};${_lib_loc})
        else()
            set(LIBRARIES_D ${LIBRARIES_D};${_lib})
        endif()
    endforeach()

    mz_debug_message("LIBRARIES=${LIBRARIES}")
    mz_debug_message("LIBRARIES_D=${LIBRARIES_D}")

    if(WIN32)
        # On Windows you must add aditional formatting to the LIBRARIES variable as a single string for the windows libtool
        # with each library path wrapped in "" in case it contains spaces
        string(REPLACE ";" "\" \"" LIBS "${LIBRARIES}")
        string(REPLACE ";" "\" \"" LIBS_D "${LIBRARIES_D}")
        set(LIBS \"${LIBS}\")
        set(LIBS_D \"${LIBS_D}\")

        if(${CONFIGURATION} STREQUAL "DEBUG")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_DEBUG "${LIBS_D}")
        elseif(${CONFIGURATION} STREQUAL "RELEASE")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_RELEASE "${LIBS}")
        elseif(${CONFIGURATION} STREQUAL "RELWITHDEBINFO")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "${LIBS}")
        elseif(${CONFIGURATION} STREQUAL "ALL")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS "${LIBS}")
        else()
            message(FATAL_ERROR "Be sure to set the CONFIGURATION argument to DEBUG, RELEASE or ALL")
        endif()

    elseif(APPLE AND ${CMAKE_GENERATOR} STREQUAL "Xcode")
        # iOS and OSX platforms with Xcode need slighly less formatting
        string(REPLACE ";" " " LIBS "${LIBRARIES}")

        if(${CONFIGURATION} STREQUAL "DEBUG")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_DEBUG "${LIBS_D}")
        elseif(${CONFIGURATION} STREQUAL "RELEASE")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_RELEASE "${LIBS}")
        elseif(${CONFIGURATION} STREQUAL "RELWITHDEBINFO")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "${LIBS}")
        elseif(${CONFIGURATION} STREQUAL "ALL")
            set_property(TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS "${LIBS}")
        else()
            message(FATAL_ERROR "Be sure to set the CONFIGURATION argument to DEBUG, RELEASE or ALL")
        endif()
    elseif(UNIX)
        # Posix platforms, including Android, require manual merging of static libraries via a special script
        set(LIBRARIES ${LIBRARIES})

        if(NOT CMAKE_BUILD_TYPE)
            message(FATAL_ERROR "To use the MergeStaticLibraries script on Posix systems, you MUST define your CMAKE_BUILD_TYPE")
        endif(NOT CMAKE_BUILD_TYPE)

        set(MERGE OFF)

        # We need the debug postfix on posix systems for the merge script
        string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
        if(${BUILD_TYPE} STREQUAL ${CONFIGURATION} OR ${CONFIGURATION} STREQUAL "ALL")
            if(${BUILD_TYPE} STREQUAL "DEBUG")
                set(LIBRARIES ${LIBRARIES_D})
                get_target_property (TARGETLOC ${TARGET} LOCATION_DEBUG)
            else()
                get_target_property (TARGETLOC ${TARGET} LOCATION)
            endif()

            set(MERGE ON)
        endif(${BUILD_TYPE} STREQUAL ${CONFIGURATION} OR ${CONFIGURATION} STREQUAL "ALL")

        # Setup the static library merge script
        if(NOT MERGE)
            message(STATUS "MergeStaticLibraries ignores mismatch betwen BUILD_TYPE=${BUILD_TYPE} and CONFIGURATION=${CONFIGURATION}")
        else(NOT MERGE)
            configure_file(
                ${MZ_TOOLS_PATH}/Modules/PosixMergeStaticLibraries.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/PosixMergeStaticLibraries-${TARGET}.cmake @ONLY
            )
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/PosixMergeStaticLibraries-${TARGET}.cmake
            )
        endif(NOT MERGE)
    endif(WIN32)

endmacro(mz_merge_static_libraries)
