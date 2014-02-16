##
# Copyright (c) 2008-2012 Marius Zwicker
# All rights reserved.
# 
# @LICENSE_HEADER_START:Apache@
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
# http://www.mlba-team.de
# 
# @LICENSE_HEADER_END:Apache@
##

FIND_PACKAGE(xdispatch)
if(NOT XDISPATCH_FOUND)
    return()
endif()

FIND_PATH(
	QTDISPATCH_INCLUDE_DIRS
	NAMES
	QtDispatch/qdispatch.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
	$ENV{SystemDrive}/xdispatch/include
	$ENV{ProgramFiles}/xdispatch/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
	QTDISPATCH_LIBRARY_DEBUG
	NAMES
	QtDispatchD libQtDispatchD
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/xdispatch/lib
	$ENV{ProgramFiles}/xdispatch/lib
)

FIND_LIBRARY(
	QTDISPATCH_LIBRARY_RELEASE
	NAMES
	QtDispatch libQtDispatch
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/xdispatch/lib
	$ENV{ProgramFiles}/xdispatch/lib
)

IF (QTDISPATCH_LIBRARY_DEBUG AND NOT QTDISPATCH_LIBRARY_RELEASE)
	SET(QTDISPATCH_LIBRARIES ${QTDISPATCH_LIBRARY_DEBUG} ${XDISPATCH_LIBRARY_DEBUG})
ENDIF ()

IF (QTDISPATCH_LIBRARY_RELEASE AND NOT QTDISPATCH_LIBRARY_DEBUG)
	SET(QTDISPATCH_LIBRARIES ${QTDISPATCH_LIBRARY_RELEASE} ${XDISPATCH_LIBRARY_RELEASE})
ENDIF ()

IF (QTDISPATCH_LIBRARY_DEBUG AND QTDISPATCH_LIBRARY_RELEASE)
	SET(QTDISPATCH_LIBRARIES debug ${QTDISPATCH_LIBRARY_DEBUG} optimized ${QTDISPATCH_LIBRARY_RELEASE})
ENDIF ()

set(QTDISPATCH_INCLUDE_DIRS ${QTDISPATCH_INCLUDE_DIRS} ${XDISPATCH_INCLUDE_DIRS})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	QtDispatch
	DEFAULT_MSG
	QTDISPATCH_INCLUDE_DIRS
	QTDISPATCH_LIBRARIES
)

MARK_AS_ADVANCED(
	QTDISPATCH_INCLUDE_DIRS
	QTDISPATCH_LIBRARIES
	QTDISPATCH_LIBRARY_DEBUG
	QTDISPATCH_LIBRARY_RELEASE
) 
