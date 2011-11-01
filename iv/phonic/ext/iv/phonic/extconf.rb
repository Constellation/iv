require 'mkmf'
require 'fileutils'
include FileUtils::Verbose
# $CFLAGS += " -Wall -Werror -Wno-unused-parameter "
dir_config('iv')
dir_config('netlib')
dir_config('v8')
$srcs = [
  "phonic.o",
  "third_party/netlib_dtoa/netlib_dtoa.c",
  "third_party/v8_dtoa/checks.cc",
  "third_party/v8_dtoa/conversions.cc",
  "third_party/v8_dtoa/diy-fp.cc",
  "third_party/v8_dtoa/fast-dtoa.cc",
  "third_party/v8_dtoa/platform.cc",
  "third_party/v8_dtoa/utils.cc",
  "third_party/v8_dtoa/v8-dtoa.cc"
]
$objs = [
  "phonic.o",
  "third_party/netlib_dtoa/netlib_dtoa.o",
  "third_party/v8_dtoa/checks.o",
  "third_party/v8_dtoa/conversions.o",
  "third_party/v8_dtoa/diy-fp.o",
  "third_party/v8_dtoa/fast-dtoa.o",
  "third_party/v8_dtoa/platform.o",
  "third_party/v8_dtoa/utils.o",
  "third_party/v8_dtoa/v8-dtoa.o"
]
mkdir_p File.join('third_party', 'netlib_dtoa')
mkdir_p File.join('third_party', 'v8_dtoa')
#dir_config('v8_dtoa', File.join('lib', 'phonic', 'third_party', 'v8_dtoa'))
#dir_config('netlib_dtoa', File.join('lib', 'phonic', 'third_party', 'netlib_dtoa'))
$CFLAGS += " -I../../include "

create_makefile('iv/phonic')
