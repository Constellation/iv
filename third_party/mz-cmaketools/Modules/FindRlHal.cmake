FIND_PATH(
	RL_HAL_INCLUDE_DIRS
	NAMES
	rl/hal/Device.h
	HINTS
	$ENV{HOME}/workspace/rl/src
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{USERPROFILE}/workspace/rl/src
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/src"
	$ENV{ProgramFiles}/rl/include
)

FIND_LIBRARY(
	RL_HAL_LIBRARY_DEBUG
	NAMES
	rlhald
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/hal
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/hal/Debug
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/hal/Debug"
	$ENV{ProgramFiles}/rl/lib
)

FIND_LIBRARY(
	RL_HAL_LIBRARY_RELEASE
	NAMES
	rlhal
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/hal
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/hal/Release
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/hal/Release"
	$ENV{ProgramFiles}/rl/lib
)

IF (RL_HAL_LIBRARY_DEBUG AND NOT RL_HAL_LIBRARY_RELEASE)
	SET(RL_HAL_LIBRARIES ${RL_HAL_LIBRARY_DEBUG})
ENDIF (RL_HAL_LIBRARY_DEBUG AND NOT RL_HAL_LIBRARY_RELEASE)

IF (RL_HAL_LIBRARY_RELEASE AND NOT RL_HAL_LIBRARY_DEBUG)
	SET(RL_HAL_LIBRARIES ${RL_HAL_LIBRARY_RELEASE})
ENDIF (RL_HAL_LIBRARY_RELEASE AND NOT RL_HAL_LIBRARY_DEBUG)

IF (RL_HAL_LIBRARY_DEBUG AND RL_HAL_LIBRARY_RELEASE)
	SET(RL_HAL_LIBRARIES debug ${RL_HAL_LIBRARY_DEBUG} optimized ${RL_HAL_LIBRARY_RELEASE})
ENDIF (RL_HAL_LIBRARY_DEBUG AND RL_HAL_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlHal
	DEFAULT_MSG
	RL_HAL_INCLUDE_DIRS
	RL_HAL_LIBRARIES
)

MARK_AS_ADVANCED(
	RL_HAL_INCLUDE_DIRS
	RL_HAL_LIBRARIES
	RL_HAL_LIBRARY_DEBUG
	RL_HAL_LIBRARY_RELEASE
) 
