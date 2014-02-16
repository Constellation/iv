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

FIND_PATH(
	BLOCKSRUNTIME_INCLUDE_DIRS
	NAMES
	Block.h Block_private.h
	HINTS
	/Library/Frameworks
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	/opt/local/include
	$ENV{SystemDrive}/QtDispatch/include
	$ENV{ProgramFiles}/QtDispatch/include
)

set(CMAKE_FIND_FRAMEWORK LAST)

FIND_LIBRARY(
	BLOCKSRUNTIME_LIBRARY_DEBUG
	NAMES
	BlocksRuntimeD libBlocksRuntimeD
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/BlocksRuntime/lib
	$ENV{ProgramFiles}/BlocksRuntime/lib
)

FIND_LIBRARY(
	BLOCKSRUNTIME_LIBRARY_RELEASE
	NAMES
	BlocksRuntime libBlocksRuntime
	HINTS
	/Library/Frameworks
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	/opt/local/lib	
	$ENV{SystemDrive}/BlocksRuntime/lib
	$ENV{ProgramFiles}/BlocksRuntime/lib
)

IF (BLOCKSRUNTIME_LIBRARY_DEBUG AND NOT BLOCKSRUNTIME_LIBRARY_RELEASE)
	SET(BLOCKSRUNTIME_LIBRARIES ${BLOCKSRUNTIME_LIBRARY_DEBUG})
ENDIF ()

IF (BLOCKSRUNTIME_LIBRARY_RELEASE AND NOT BLOCKSRUNTIME_LIBRARY_DEBUG)
	SET(BLOCKSRUNTIME_LIBRARIES ${BLOCKSRUNTIME_LIBRARY_RELEASE})
ENDIF ()

IF (BLOCKSRUNTIME_LIBRARY_DEBUG AND BLOCKSRUNTIME_LIBRARY_RELEASE)
	SET(BLOCKSRUNTIME_LIBRARIES debug ${BLOCKSRUNTIME_LIBRARY_DEBUG} optimized ${BLOCKSRUNTIME_LIBRARY_RELEASE})
ENDIF ()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	BlocksRuntime
	DEFAULT_MSG
	BLOCKSRUNTIME_INCLUDE_DIRS
	BLOCKSRUNTIME_LIBRARIES
)

MARK_AS_ADVANCED(
	BLOCKSRUNTIME_INCLUDE_DIRS
	BLOCKSRUNTIME_LIBRARIES
	BLOCKSRUNTIME_LIBRARY_DEBUG
	BLOCKSRUNTIME_LIBRARY_RELEASE
) 
