@ echo off
REM REM
REM Copyright (c) 2008-2012 Marius Zwicker
REM All rights reserved.
REM 
REM @LICENSE_HEADER_START:Apache@
REM 
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM 
REM     http://www.apache.org/licenses/LICENSE-2.0
REM 
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.
REM 
REM http://www.mlba-team.de
REM 
REM @LICENSE_HEADER_END:Apache@
REM REM

REM #############################################################
REM
REM		Configure for MinGW-w64 Make
REM		(c) 2012 Marius Zwicker
REM
REM		Pass 'Release' as argument to build without debug flags
REM
REM #############################################################

@echo off

set BUILD_DIR=MinGW64_Makefiles
set RELEASE_DIR=Release_%BUILD_DIR%
set GENERATOR=MinGW Makefiles
set TARGET=MinGW64/Windows

call "%~dp0\util.bat" %*