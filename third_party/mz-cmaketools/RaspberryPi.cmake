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

## File containing settings for crosscompiling Linux -> RaspberryPi (armhf)

if(NOT RASPI_ROOTFS)
  set(RASPI_ROOTFS "$ENV{RASPI_ROOT_FS}" CACHE INTERNAL "RASPI_ROOTFS")
endif()

if(NOT RASPI_TOOLCHAIN)
  set(RASPI_TOOLCHAIN "$ENV{RASPI_TOOLCHAIN}" CACHE INTERNAL "RASPI_TOOLCHAIN")
endif()

if(NOT RASPI_ROOTFS)
  message(FATAL_ERROR "Missing raspberry pi rootfs, please set RASPI_ROOT_FS env variable")
endif()

if(NOT RASPI_TOOLCHAIN)
  message(FATAL_ERROR "Missing raspberry pi toolchain, please set RASPI_TOOLCHAIN env variable")
endif()

mark_as_advanced(RASPI_ROOTFS RASPI_TOOLCHAIN)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)	
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-g++)
#set(CMAKE_AR ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-ar)
#set(CMAKE_RANLIB ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-ranlib)
#set(CMAKE_LINKER ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-ld)
#set(CMAKE_NM ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-nm)
#set(CMAKE_STRIP ${RASPI_TOOLCHAIN}/bin/arm-linux-gnueabihf-strip)
set(CMAKE_FIND_ROOT_PATH ${RASPI_ROOTFS})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(RASPI true CACHE INTERNAL "RASPI")
set(UNIX true CACHE INTERNAL "UNIX")
