##
# Copyright (c) 2013 Marius Zwicker
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

FIND_PATH(
    DISPATCH_INCLUDE_DIRS
	NAMES
	dispatch/dispatch.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
	$ENV{SystemDrive}/xdispatch/include
	$ENV{ProgramFiles}/xdispatch/include
)

if(APPLE)
	set( DISPATCH_LIBRARIES )
	set( DISPATCH_LIBRARY_RELEASE )
	set( DISPATCH_LIBRARY_DEBUG )
else()
	
	set(CMAKE_FIND_FRAMEWORK LAST)

	FIND_LIBRARY(
		DISPATCH_LIBRARY_DEBUG
		NAMES
		dispatchD libdispatchD
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
		DISPATCH_LIBRARY_RELEASE
		NAMES
		dispatch libdispatch
		HINTS
		/Library/Frameworks
		$ENV{HOME}/lib
		/usr/local/lib
		/usr/lib
		/opt/local/lib	
		$ENV{SystemDrive}/xdispatch/lib
		$ENV{ProgramFiles}/xdispatch/lib
	)

	IF (DISPATCH_LIBRARY_DEBUG AND NOT DISPATCH_LIBRARY_RELEASE)
		SET(DISPATCH_LIBRARIES ${DISPATCH_LIBRARY_DEBUG})
	ENDIF ()

	IF (DISPATCH_LIBRARY_RELEASE AND NOT DISPATCH_LIBRARY_DEBUG)
		SET(DISPATCH_LIBRARIES ${DISPATCH_LIBRARY_RELEASE})
	ENDIF ()

	IF (DISPATCH_LIBRARY_DEBUG AND DISPATCH_LIBRARY_RELEASE)
		SET(DISPATCH_LIBRARIES debug ${DISPATCH_LIBRARY_DEBUG} optimized ${DISPATCH_LIBRARY_RELEASE})
	ENDIF ()

endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	dispatch
	DEFAULT_MSG
	DISPATCH_INCLUDE_DIRS
	DISPATCH_LIBRARIES
)

MARK_AS_ADVANCED(
	DISPATCH_INCLUDE_DIRS
	DISPATCH_LIBRARIES
	DISPATCH_LIBRARY_DEBUG
	DISPATCH_LIBRARY_RELEASE
) 
