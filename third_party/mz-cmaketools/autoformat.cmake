##################################################
#
#	BUILD/AUTOFORMAT.CMAKE
#
# 	Provides an easy mean to reformat a file using
#   uncrustify and the settings as defined in autoformat.cfg
#
#	Copyright (c) 2013 Marius Zwicker
#
#
#
# PROVIDED MACROS
# -----------------------
# mz_auto_format <target> [<file1> <file2>]...
#   Format the sourcefiles whenever the given target is built.
#   When no explicit sourcefiles are given, all sources the target
#   depends on and ending with (cxx|hpp|cpp|c) will be automatically
#   marked for autoformat
#
########################################################################


########################################################################
## no need to change anything beyond here
########################################################################

find_program(
    MZ_UNCRUSTIFY_BIN
    uncrustify
)
if(NOT WINDOWS)
  set(MZ_CPPLINT_BIN ${MZ_TOOLS_PATH}/cpplint.py CACHE PATH "Path to cpplint" FORCE)
endif()

if( MZ_IS_RELEASE )
    option(MZ_DO_AUTO_FORMAT "Enable to run autoformat on configured targets" OFF)
    option(MZ_DO_CPPLINT "Enable to run cpplint on configured targets" OFF)
else()
    option(MZ_DO_AUTO_FORMAT "Enable to run autoformat on configured targets" ON)
    option(MZ_DO_CPPLINT "Enable to run cpplint on configured targets" ON)
endif()

macro(mz_auto_format _TARGET)
  set(_sources ${ARGN})
  list(LENGTH _sources arg_count)
  configure_file(${MZ_TOOLS_PATH}/autoformat.cfg.in ${CMAKE_BINARY_DIR}/autoformat.cfg)

  if( NOT arg_count GREATER 0 )
    mz_debug_message("Autoformat was no files given, using the target's sources")
    get_target_property(_sources ${_TARGET} SOURCES)
  endif()

  # remove readability/alt_tokens again when the bug of cpplint detecting "and" within comments is fixed
  set(CPPLINT_FILTERS
    -whitespace,-build/header_guard,-build/include,+build/include_what_you_use,-readability/multiline_comment,-readability/namespace,-readability/streams,-runtime/references,-runtime/threadsafe_fn,-readability/alt_tokens
  )

  set(_sources2 "")
  foreach(file ${_sources})
    get_filename_component(abs_file ${file} ABSOLUTE)
    if( ${file} MATCHES ".+\\.(cpp|cxx|hpp|h|c|vert|glsl|frag|cl)$" AND NOT ${file} MATCHES "(ui_|moc_|qrc_).+" )
        set(_sources2 ${_sources2} ${abs_file})
    endif()
  endforeach()

  set(_new_target_format ${_TARGET}_autoformat)
  set(_new_target_lint ${_TARGET}_cpplint)

  if( MZ_UNCRUSTIFY_BIN AND MZ_DO_AUTO_FORMAT )
    add_library(${_new_target_format} STATIC ${CMAKE_CURRENT_BINARY_DIR}/format_step.c)
    set_target_properties(${_new_target_format} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/format_step.c
        COMMAND ${MZ_UNCRUSTIFY_BIN} -c ${CMAKE_BINARY_DIR}/autoformat.cfg --no-backup --mtime ${_sources2}
        COMMAND ${CMAKE_COMMAND} -E copy ${MZ_TOOLS_PATH}/autoformat.c.in ${CMAKE_CURRENT_BINARY_DIR}/format_step.c
        DEPENDS ${_sources}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
  endif()
  if( MZ_CPPLINT_BIN AND MZ_DO_CPPLINT AND NOT __MZ_NO_CPPLINT )
    add_library(${_new_target_lint} STATIC ${CMAKE_CURRENT_BINARY_DIR}/lint_step.c)
    set_target_properties(${_new_target_lint} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lint_step.c
        COMMAND ${MZ_CPPLINT_BIN} --root=${CMAKE_CURRENT_LIST_DIR} --filter=${CPPLINT_FILTERS} --output=eclipse ${_sources2}
        COMMAND ${CMAKE_COMMAND} -E copy ${MZ_TOOLS_PATH}/autoformat.c.in ${CMAKE_CURRENT_BINARY_DIR}/lint_step.c
        DEPENDS ${_sources}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
  endif()

  mz_debug_message("NEW_TARGET=${_new_target_format};${_new_target_lint}, WORKING_DIRECTORY=${CMAKE_CURRENT_LIST_DIR}")
endmacro()

macro(mz_auto_format_c _TARGET)
   set(__MZ_NO_CPPLINT TRUE)
   mz_auto_format(${_TARGET} ${ARGN})
endmacro()

macro(mz_auto_format_cxx _TARGET)
   set(__MZ_NO_CPPLINT TRUE)
   mz_auto_format(${_TARGET} ${ARGN})
endmacro()
