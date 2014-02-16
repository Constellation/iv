########################################################################
#
#	BUILD/MACROS.CMAKE
#
# 	This file provides some useful macros to
#	simplify adding of componenents and other
#	tasks
#	Copyright (c) 2009-2013 Marius Zwicker
#
# This file defines a whole bunch of macros
# to add a subdirectory containing another
# CMakeLists.txt as "Subproject". All these
# Macros are not doing that much but giving
# feedback to tell what kind of component was
# added. In all cases NAME is the name of your
# subproject and FOLDER is a relative path to
# the folder containing a CMakeLists.txt
#
# mz_add_library <NAME> <FOLDER>
#		macro for adding a new library
#
# mz_add_executable <NAME> <FOLDER>
# 		macro for adding a new executable
#
# mz_add_control <NAME> <FOLDER>
#		macro for adding a new control
#
# mz_add_testtool <NAME> <FOLDER>
#		macro for adding a folder containing testtools
#
# mz_add_external <NAME> <FOLDER>
#		macro for adding an external library/tool dependancy
#
# mz_target_props <target>
#		automatically add a "D" postfix when compiling in debug
#       mode to the given target
#
# mz_auto_moc <mocced> ...
#		search all passed files in (...) for Q_OBJECT and if found
#		run moc on them via qt4_wrap_cpp. Assign the output files
#		to <mocced>. Improves the version provided by cmake by searching
#       for Q_OBJECT first and thus reducing the needed calls to moc
#
# mz_find_include_library <name>  SYS <version> SRC <directory> <include_dir> <target>
#       useful when providing a version of a library within the
#       own sourcetree but prefer the system's library version over it.
#       Will search for the given library in the system using find_package and when
#       not found, it will include the given directory which should contain
#       a cmake file defining the given target.
#       After calling this macro the following variables will be declared:
#           <name>_INCLUDE_DIRS The directory containing the header or 
#                              the passed include_dir if the lib was not 
#                              found on the system
#           <name>_LIBRARIES The libs to link against - either lib or target
#           <name>_FOUND true if the lib was found on the system
#           <name>_IGNORE_SYSTEM_LIBRARY Will be an option available
#                              for custom configuration. Enable to ignore the
#                              detected system install library and force use
#                              of the version in <directory>
#
# mz_find_checkout_library <name>  SYS <version> SVN <repository> <target_dir> <include_dir> <target>
#       useful when needing a version of a library but not sure wether
#       it is available on the target system.
#       Will search for the given library in the system using find_package and when
#       not found, it will do a checkout of the given repository which should contain
#       a cmake file defining the given target.
#       After calling this macro the following variables will be declared:
#           <name>_INCLUDE_DIRS The directory containing the header or
#                              the passed include_dir if the lib was not
#                              found on the system
#           <name>_LIBRARIES The libs to link against - either lib or target
#           <name>_FOUND true if the lib was found on the system
#           <name>_IGNORE_SYSTEM_LIBRARY Will be an option available
#                              for custom configuration. Enable to ignore the
#                              detected system install library and force use
#                              of the version in <directory>
#
########################################################################

# if global.cmake was not included yet, report it
if (NOT HAS_MZ_GLOBAL)
    message(FATAL_ERROR "!! include global.cmake before including this file !!")
endif()

########################################################################
## no need to change anything beyond here
########################################################################

macro(mz_add_library NAME FOLDER)
    mz_message("adding library ${NAME}")
    __mz_add_target(${NAME} ${FOLDER})
endmacro()

macro(mz_add_executable NAME FOLDER)
    mz_message("adding executable ${NAME}")
    __mz_add_target(${NAME} ${FOLDER})
endmacro()

macro(mz_add_control NAME FOLDER)
    mz_message("adding control ${NAME}")
    __mz_add_target(${NAME} ${FOLDER})
endmacro()

macro(mz_add_testtool NAME FOLDER)
    mz_message("adding testtool ${NAME}")
    __mz_add_target(${NAME} ${FOLDER})
endmacro()

macro(mz_add_external NAME FOLDER)
    mz_message("adding external dependancy ${NAME}")
    __mz_add_target(${NAME} ${FOLDER})
endmacro()

macro(__mz_add_target NAME FOLDER)
    get_filename_component(_ABS_FOLDER ${FOLDER} ABSOLUTE)
    file(RELATIVE_PATH _REL_FOLDER ${CMAKE_SOURCE_DIR} ${_ABS_FOLDER})

    add_subdirectory(${FOLDER} ${CMAKE_BINARY_DIR}/${_REL_FOLDER})
endmacro()

macro(mz_target_props NAME)
    set_target_properties(${NAME} PROPERTIES DEBUG_POSTFIX "D")
endmacro()

macro(__mz_extract_files _qt_files)
    set(${_qt_files})
    FOREACH(_current ${ARGN})
            file(STRINGS ${_current} _content LIMIT_COUNT 1 REGEX .*Q_OBJECT.*)
            if("${_content}" MATCHES .*Q_OBJECT.*)
                    LIST(APPEND ${_qt_files} "${_current}")
            endif()
    ENDFOREACH(_current)
endmacro()

macro(mz_auto_moc mocced)
    #mz_debug_message("mz_auto_moc input: ${ARGN}")

    set(_mocced "")
    # determine the required files
    __mz_extract_files(to_moc ${ARGN})
    mz_debug_message("mz_auto_moc mocced in: ${to_moc}")
    # the definition of -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED is to bypass a parsing bug within moc
    if( Qt5Core_VERSION_STRING )
        qt5_wrap_cpp(_mocced ${to_moc} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED -DBOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    else()
        qt4_wrap_cpp(_mocced ${to_moc} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED -DBOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    endif()
	set(${mocced} ${${mocced}} ${_mocced})
endmacro()

include(CheckIncludeFiles)
include(FindPackageHandleStandardArgs)

if( NOT CMAKE_MODULE_PATH )
    cmake_policy(SET CMP0017 OLD)
    set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules" )
endif()

macro(mz_check_include_files FILE VAR)
    if( IOS )
        mz_debug_message("Using custom check_include_files")

        if( NOT DEFINED FOUND_${VAR} )
            mz_message("Looking for include files ${FILE}")
            find_file( ${VAR}
                    NAMES ${FILE}
                    PATHS ${CMAKE_REQUIRED_INCLUDES}
            )
            if( ${VAR} )
                    mz_message("Looking for include files ${FILE} - found")
                    set( FOUND_${VAR} ${${VAR}} CACHE INTERNAL FOUND_${VAR} )
            else()
                    mz_message("Looking for include files ${FILE} - not found")
            endif()
        else()
            set( ${VAR} ${FOUND_${VAR}} )
        endif()

    else()
        mz_debug_message("Using native check_include_files")

        check_include_files( ${FILE} ${VAR} )
    endif()
endmacro()

macro(mz_find_include_library _NAME SYS _VERSION SRC _DIRECTORY _INC_DIR _TARGET)
    
    STRING(TOUPPER ${_NAME} _NAME_UPPER2)
    STRING(REPLACE "-" "_" _NAME_UPPER "${_NAME_UPPER2}") # special care for libraries with - in their names
    get_filename_component(_DIRECTORY_ABS ${_DIRECTORY} ABSOLUTE)

    # we only search for the library in case
    # - the given target was not defined before (think hierarchies)
    if( NOT TARGET ${_TARGET} )
        find_package( ${_NAME} ${_VERSION} QUIET )
    endif()

    # take care of find_package not converting to upper-case
    if( ${_NAME}_FOUND OR ${_NAME_UPPER2}_FOUND )
        set( ${_NAME_UPPER}_FOUND TRUE )
    endif()

    # give some choice to the user
    if( ${_NAME_UPPER}_FOUND )
        option(${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY "Force use of the in-source library version of ${_NAME}" OFF)
    else()
        option(${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY "Force use of the in-source library version of ${_NAME}" ON)
    endif()

    # we only add the library as our own target in case
    # - no system library was found
    # - the given target was not defined before (think hierarchies)
    # - the use explicitly wants to build the library himself
    if( ( NOT ${_NAME_UPPER}_FOUND OR ${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY ) AND NOT TARGET ${_TARGET} )
        get_filename_component(_INC_DIR_ABS ${_INC_DIR} ABSOLUTE)
        set(${_NAME_UPPER}_INCLUDE_DIRS ${_INC_DIR_ABS})
        set(${_NAME_UPPER}_LIBRARIES ${_TARGET} ${ARGN})
        set(${_NAME_UPPER}_FOUND TRUE)
        
        mz_message("No system library for '${_NAME}', building own version")
        if( NOT ${_TARGET} STREQUAL "" )
            mz_add_library(${_NAME} ${_DIRECTORY})
        else()
            mz_debug_message("${_NAME} is a header only library, no new target")
        endif()
    else()
        mz_message("Using system installed version of '${_NAME}'")
    endif()

endmacro()

macro(mz_find_checkout_library _NAME SYS _VERSION SVN _REPOSITORY _DEST_DIR _INC_DIR _TARGET)

    STRING(TOUPPER ${_NAME} _NAME_UPPER2)
    STRING(REPLACE "-" "_" _NAME_UPPER "${_NAME_UPPER2}") # special care for libraries with - in their names
    get_filename_component(_DEST_DIR_ABS ${_DEST_DIR} ABSOLUTE)

    # we only search for the library in case
    # - the given target was not defined before (think hierarchies)
    if( NOT TARGET ${_TARGET} )
        find_package( ${_NAME} ${_VERSION} QUIET )
    endif()

    # take care of find_package not converting to upper-case
    if( ${_NAME}_FOUND OR ${_NAME_UPPER2}_FOUND )
        set( ${_NAME_UPPER}_FOUND TRUE )
    endif()

    # give some choice to the user
    if( ${_NAME_UPPER}_FOUND )
        option(${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY "Force use of the in-source library version of ${_NAME}" OFF)
    else()
        option(${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY "Force use of the in-source library version of ${_NAME}" ON)
    endif()

    # we only add the library as our own target in case
    # - no system library was found
    # - the given target was not defined before (think hierarchies)
    # - the use explicitly wants to build the library himself
    if( ( NOT ${_NAME_UPPER}_FOUND OR ${_NAME_UPPER}_IGNORE_SYSTEM_LIBRARY ) AND NOT TARGET ${_TARGET} )
        mz_message("No system library for '${_NAME}', retrieving own version")

        find_package( Subversion QUIET )
        if( SUBVERSION_FOUND )
            if(NOT EXISTS ${_DEST_DIR_ABS})
                execute_process(
                    COMMAND ${Subversion_SVN_EXECUTABLE} checkout ${_REPOSITORY} ${_DEST_DIR_ABS}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    OUTPUT_VARIABLE _SVN_OUT
                )
                mz_debug_message( ${_SVN_OUT} )
            endif()
        else()
            mz_message( "WARNING: No Subversion found, please do a manual checkout of ${_REPOSITORY} to ${_DEST_DIR_ABS} and re-run project generation" )
        endif()

        if(NOT EXISTS ${_DEST_DIR_ABS})
            mz_error_message( "Failed to include ${_DEST_DIR_ABS} as library source, please make sure a valid checkout exists there" )
        endif()

        set(${_NAME_UPPER}_INCLUDE_DIRS "")
        foreach(_DIR ${_INC_DIR})
            get_filename_component(_DIR_ABS ${_DIR} ABSOLUTE)
            list(APPEND ${_NAME_UPPER}_INCLUDE_DIRS "${_DIR_ABS}")
            mz_debug_message("Adding ${_DIR_ABS} to ${${_NAME_UPPER}_INCLUDE_DIRS}")
        endforeach(_DIR)
        set(${_NAME_UPPER}_LIBRARIES ${_TARGET} ${ARGN})
        set(${_NAME_UPPER}_FOUND TRUE)

        if( NOT ${_TARGET} STREQUAL "" )
            mz_add_library(${_NAME} ${_DEST_DIR})
        else()
            mz_debug_message("${_NAME} is a header only library, no new target")
        endif()
    else()
        mz_message("Using system installed version of '${_NAME}'")
    endif()

endmacro()

