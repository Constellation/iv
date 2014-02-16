#!/bin/bash

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

#######################################################################
#
#  Configure Codeblocks/QtCreator project files
#  (c) 2012 Marius Zwicker
#
#  Pass 'Release' as argument to build without debug flags
#
#######################################################################

BUILD_DIR="QtCreator_ProjectFiles"
RELEASE_DIR="Release_$BUILD_DIR"
GENERATOR="CodeBlocks - Unix Makefiles"
TARGET="Qt Creator"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $DIR/util.sh $@