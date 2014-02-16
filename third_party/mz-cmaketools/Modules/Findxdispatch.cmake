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

FIND_PACKAGE(dispatch)
if(NOT DISPATCH_FOUND)
    return()
endif()

FIND_PATH(
	XDISPATCH_INCLUDE_DIRS
	NAMES
	xdispatch/dispatch.h
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
	XDISPATCH_LIBRARY_DEBUG
	NAMES
	xdispatchD libxdispatchD
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
	XDISPATCH_LIBRARY_RELEASE
	NAMES
	xdispatch libxdispatch
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/xdispatch/lib
	$ENV{ProgramFiles}/xdispatch/lib
)

IF (XDISPATCH_LIBRARY_DEBUG AND NOT XDISPATCH_LIBRARY_RELEASE)
	SET(XDISPATCH_LIBRARIES ${XDISPATCH_LIBRARY_DEBUG} ${DISPATCH_LIBRARY_DEBUG})
ENDIF ()

IF (XDISPATCH_LIBRARY_RELEASE AND NOT XDISPATCH_LIBRARY_DEBUG)
	SET(XDISPATCH_LIBRARIES ${XDISPATCH_LIBRARY_RELEASE} ${DISPATCH_LIBRARY_RELEASE})
ENDIF ()

IF (XDISPATCH_LIBRARY_DEBUG AND XDISPATCH_LIBRARY_RELEASE)
	SET(XDISPATCH_LIBRARIES debug ${XDISPATCH_LIBRARY_DEBUG} optimized ${XDISPATCH_LIBRARY_RELEASE})
ENDIF ()

set(XDISPATCH_INCLUDE_DIRS ${XDISPATCH_INCLUDE_DIRS} ${DISPATCH_INCLUDE_DIRS})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	xdispatch
	DEFAULT_MSG
	XDISPATCH_INCLUDE_DIRS
	XDISPATCH_LIBRARIES
)

MARK_AS_ADVANCED(
	XDISPATCH_INCLUDE_DIRS
	XDISPATCH_LIBRARIES
	XDISPATCH_LIBRARY_DEBUG
	XDISPATCH_LIBRARY_RELEASE
) 
