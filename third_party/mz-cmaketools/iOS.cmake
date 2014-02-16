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

# This file is based off of the Platform/Darwin.cmake and Platform/UnixPaths.cmake
# files which are included with CMake 2.8.4
# It has been altered for iOS development

# Options:
#
# IOS_PLATFORM = OS (default) or SIMULATOR
#   This decides if SDKS will be selected from the iPhoneOS.platform or iPhoneSimulator.platform folders
#   OS - the default, used to build for iPhone and iPad physical devices, which have an arm arch.
#   SIMULATOR - used to build for the Simulator platforms, which have an x86 arch.
#
# CMAKE_IOS_DEVELOPER_ROOT = automatic(default) or /path/to/platform/Developer folder
#   By default this location is automatcially chosen based on the IOS_PLATFORM value above.
#   If set manually, it will override the default location and force the user of a particular Developer Platform
#
# CMAKE_IOS_SDK_ROOT = automatic(default) or /path/to/platform/Developer/SDKs/SDK folder
#   By default this location is automatcially chosen based on the CMAKE_IOS_DEVELOPER_ROOT value.
#   In this case it will always be the most up-to-date SDK found in the CMAKE_IOS_DEVELOPER_ROOT path.
#   If set manually, this will force the use of a specific SDK version

# Standard settings
set (CMAKE_SYSTEM_NAME Darwin)
set (CMAKE_SYSTEM_VERSION 1)
set (CMAKE_SYSTEM_PROCESSOR ARM)
set (UNIX True)
set (APPLE True)
set (IOS True)

# Determine the cmake host system version so we know where to find the iOS SDKs
find_program (CMAKE_UNAME uname /bin /usr/bin /usr/local/bin)
if (CMAKE_UNAME)
	exec_program(uname ARGS -r OUTPUT_VARIABLE CMAKE_HOST_SYSTEM_VERSION)
	string (REGEX REPLACE "^([0-9]+)\\.([0-9]+).*$" "\\1" DARWIN_MAJOR_VERSION "${CMAKE_HOST_SYSTEM_VERSION}")
endif (CMAKE_UNAME)

# All iOS/Darwin specific settings - some may be redundant
set (CMAKE_SHARED_LIBRARY_PREFIX "lib")
set (CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
set (CMAKE_SHARED_MODULE_PREFIX "lib")
set (CMAKE_SHARED_MODULE_SUFFIX ".so")
set (CMAKE_MODULE_EXISTS 1)
set (CMAKE_DL_LIBS "")

set (CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "-compatibility_version ")
set (CMAKE_C_OSX_CURRENT_VERSION_FLAG "-current_version ")
set (CMAKE_CXX_OSX_COMPATIBILITY_VERSION_FLAG "${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}")
set (CMAKE_CXX_OSX_CURRENT_VERSION_FLAG "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}")

# Hidden visibilty is required for cxx on iOS 
set (CMAKE_C_FLAGS_INIT "")
set (CMAKE_CXX_FLAGS_INIT "-headerpad_max_install_names -fvisibility=hidden -fvisibility-inlines-hidden")

set (CMAKE_C_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_C_LINK_FLAGS}")
set (CMAKE_CXX_LINK_FLAGS "-Wl,-search_paths_first ${CMAKE_CXX_LINK_FLAGS}")

set (CMAKE_PLATFORM_HAS_INSTALLNAME 1)
set (CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -headerpad_max_install_names")
set (CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -headerpad_max_install_names")
set (CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
set (CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
set (CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".so" ".a")

# hack: if a new cmake (which uses CMAKE_INSTALL_NAME_TOOL) runs on an old build tree
# (where install_name_tool was hardcoded) and where CMAKE_INSTALL_NAME_TOOL isn't in the cache
# and still cmake didn't fail in CMakeFindBinUtils.cmake (because it isn't rerun)
# hardcode CMAKE_INSTALL_NAME_TOOL here to install_name_tool, so it behaves as it did before, Alex
if (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
	find_program(CMAKE_INSTALL_NAME_TOOL install_name_tool)
endif (NOT DEFINED CMAKE_INSTALL_NAME_TOOL)

# Setup iOS platform
if (NOT DEFINED IOS_PLATFORM)
	set (IOS_PLATFORM "OS")
endif (NOT DEFINED IOS_PLATFORM)
set (IOS_PLATFORM ${IOS_PLATFORM} CACHE STRING "Type of iOS Platform")

# Check the platform selection and setup for developer root
if (${IOS_PLATFORM} STREQUAL "OS")
	set (IOS_PLATFORM_LOCATION "iPhoneOS.platform")

	# This causes the installers to properly locate the output libraries
	set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
elseif (${IOS_PLATFORM} STREQUAL "SIMULATOR")
	set (IOS_PLATFORM_LOCATION "iPhoneSimulator.platform")

	# This causes the installers to properly locate the output libraries
	set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
else (${IOS_PLATFORM} STREQUAL "OS")
	message (FATAL_ERROR "Unsupported IOS_PLATFORM value selected. Please choose OS or SIMULATOR")
endif (${IOS_PLATFORM} STREQUAL "OS")

# Setup iOS developer location, which changed for OSX Lion (Darwin 11)
if (NOT DEFINED CMAKE_IOS_DEVELOPER_ROOT)
	if (${DARWIN_MAJOR_VERSION} GREATER "10")
		set (CMAKE_IOS_DEVELOPER_ROOT "/Applications/Xcode.app/Contents/Developer/Platforms/${IOS_PLATFORM_LOCATION}/Developer")
	else (${DARWIN_MAJOR_VERSION} GREATER "10")
		set (CMAKE_IOS_DEVELOPER_ROOT "/Developer/Platforms/${IOS_PLATFORM_LOCATION}/Developer")
	endif (${DARWIN_MAJOR_VERSION} GREATER "10")
endif (NOT DEFINED CMAKE_IOS_DEVELOPER_ROOT)
set (CMAKE_IOS_DEVELOPER_ROOT ${CMAKE_IOS_DEVELOPER_ROOT} CACHE PATH "Location of iOS Platform")

# Find and use the most recent iOS sdk 
if (NOT DEFINED CMAKE_IOS_SDK_ROOT)
	file (GLOB _CMAKE_IOS_SDKS "${CMAKE_IOS_DEVELOPER_ROOT}/SDKs/*")
	if (_CMAKE_IOS_SDKS) 
		list (SORT _CMAKE_IOS_SDKS)
		list (REVERSE _CMAKE_IOS_SDKS)
		list (GET _CMAKE_IOS_SDKS 0 CMAKE_IOS_SDK_ROOT)
	else (_CMAKE_IOS_SDKS)
		message (FATAL_ERROR "No iOS SDK's found in default seach path ${CMAKE_IOS_DEVELOPER_ROOT}. Manually set CMAKE_IOS_SDK_ROOT or install the iOS SDK.")
	endif (_CMAKE_IOS_SDKS)
	message (STATUS "Toolchain using default iOS SDK: ${CMAKE_IOS_SDK_ROOT}")
endif (NOT DEFINED CMAKE_IOS_SDK_ROOT)
set (CMAKE_IOS_SDK_ROOT ${CMAKE_IOS_SDK_ROOT} CACHE PATH "Location of the selected iOS SDK")

# Set the sysroot default to the most recent SDK
set (CMAKE_OSX_SYSROOT ${CMAKE_IOS_SDK_ROOT} CACHE PATH "Sysroot used for iOS support")

# set the architecture for iOS - using ARCHS_STANDARD_32_BIT sets armv6,armv7 and appears to be XCode's standard. 
# The other value that works is ARCHS_UNIVERSAL_IPHONE_OS but that sets armv7 only
set (CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_32_BIT)" CACHE string  "Build architecture for iOS")


# Skip the platform compiler checks for cross compiling
set (CMAKE_CXX_COMPILER_WORKS TRUE)
set (CMAKE_C_COMPILER_WORKS TRUE)

# Force the compilers to gcc for iOS
include (CMakeForceCompiler)
#CMAKE_FORCE_C_COMPILER (gcc ${CMAKE_IOS_DEVELOPER_ROOT}/usr/bin/gcc)
#CMAKE_FORCE_CXX_COMPILER (g++ ${CMAKE_IOS_DEVELOPER_ROOT}/usr/bin/++)
SET(CMAKE_C_COMPILER   ${CMAKE_IOS_DEVELOPER_ROOT}/usr/bin/gcc)
SET(CMAKE_CXX_COMPILER ${CMAKE_IOS_DEVELOPER_ROOT}/usr/bin/g++)


if (NOT XCODE)
          # Enable shared library versioning.  This flag is not actually referenced
          # but the fact that the setting exists will cause the generators to support
          # soname computation.
          set (CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-install_name")
endif (NOT XCODE)

# Xcode does not support -isystem yet.
if (XCODE)
        set (CMAKE_INCLUDE_SYSTEM_FLAG_C)
        set (CMAKE_INCLUDE_SYSTEM_FLAG_CXX)
endif (XCODE)

# Set the find root to the iOS developer roots and to user defined paths
set (CMAKE_FIND_ROOT_PATH ${CMAKE_IOS_DEVELOPER_ROOT} ${CMAKE_IOS_SDK_ROOT} ${CMAKE_PREFIX_PATH} CACHE string  "iOS find search path root")

# default to searching for frameworks first
set (CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set (CMAKE_SYSTEM_FRAMEWORK_PATH
	${CMAKE_IOS_SDK_ROOT}/System/Library/Frameworks
	${CMAKE_IOS_SDK_ROOT}/System/Library/PrivateFrameworks
	${CMAKE_IOS_SDK_ROOT}/Developer/Library/Frameworks
)

# only search the iOS sdks, not the remainder of the host filesystem
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set (CMAKE_REQUIRED_INCLUDES ${CMAKE_IOS_SDK_ROOT}/usr/include ${CMAKE_IOS_DEVELOPER_ROOT}/usr/include CACHE string  "iOS find search path include")