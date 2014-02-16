MZ CMake Tools
==============

Provides some useful extensions to the normal 
functionality of CMake


Additional Modules
------------------

When using the mz tools more libraries can be 
searched for, see the Modules directory for details.


Macros and Functions
--------------------

Additional commands, some are nothing more
than cosmetic...

### mz_add_definition <definition1> ...
add the definition <definition> (and following)
to the list of definitions passed to the compiler.
Automatically switches between the syntax of msvc 
and gcc/clang
Example: mz_add_definition(NO_DEBUG)

### mz_add_cxx_flag GCC|CLANG|VS|ALL <flag1> <flag2> ...
pass the given flag to the C++ compiler when
the compiler matches the given platform

### mz_add_c_flag GCC|CLANG|VS|ALL <flag1> <flag2> ...
pass the given flag to the C compiler when
the compiler matches the given platform

### mz_add_flag GCC|CLANG|VS|ALL <flag1> <flag2> ...
pass the given flag to the compiler, no matter
wether compiling C or C++ files. The selected platform
is still respected

### mz_use_default_compiler_settings
resets all configured compiler flags back to the
cmake default. This is especially useful when adding
external libraries which might still have compiler warnings


Provided CMake Variables
------------------------
MZ_IS_VS true when the platform is MS Visual Studio

MZ_IS_GCC true when the compiler is gcc or compatible

MZ_IS_CLANG true when the compiler is clang

MZ_IS_XCODE true when configuring for the XCode IDE

MZ_IS_RELEASE true when building with CMAKE_BUILD_TYPE = "Release"

MZ_64BIT true when building for a 64bit system

MZ_32BIT true when building for a 32bit system

MZ_HAS_CXX0X see MZ_HAS_CXX11

MZ_HAS_CXX11 true when the compiler supports at least a
             (subset) of the upcoming C++11 standard

DARWIN true when building on OS X / iOS

IOS true when building for iOS

WINDOWS true when building on Windows

LINUX true when building on Linux

MZ_DATE_STRING a string containing day, date and time of the
               moment cmake was executed
               e.g. Mo, 27 Feb 2012 19:47:23 +0100


Enabled compiler definitions/options
------------------------------------
On all compilers supporting it, the option to treat warnings
will be set. Additionally the warn level of the compiler will
be decreased. See mz_use_default_compiler_settings whenever some
warnings have to be accepted

### Provided defines (defined to 1)

WINDOWS / WIN32 on Windows

LINUX on Linux

DARWIN on Darwin / OS X / iOS

IOS on iOS

WIN32_VS on MSVC - note this is deprecated, it is recommended to use _MSC_VER

WIN32_MINGW when using the mingw toolchain

WIN32_MINGW64 when using the mingw-w64 toolchain

MZ_HAS_CXX11 / MZ_HAS_CXX0X when subset of C++11 is available


Installation and Usage
----------------------
Simply copy all files including the Module directory
into your project and include it within your CMakelists.txt
by typing

    include(< your folder >/global.cmake)

All settings will be done automatically and the
given functions can be directly used
