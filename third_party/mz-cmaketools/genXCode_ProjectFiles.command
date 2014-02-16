#!/bin/sh
#######################################################################
#
#  Configure XCode project files
#  (c) 2012 Marius Zwicker
#
#  Pass 'Release' as argument to build without debug flags
#
#######################################################################

BUILD_DIR="XCode_ProjectFiles"
RELEASE_DIR="Release_$BUILD_DIR"
GENERATOR="Xcode"
TARGET="Darwin OSX X64"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $DIR/util.sh $@