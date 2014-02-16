FIND_PATH(
	RL_PLAN_INCLUDE_DIRS
	NAMES
	rl/plan/Planner.h
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
	RL_PLAN_LIBRARY_DEBUG
	NAMES
	rlpland
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/plan
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/plan/Debug
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/plan/Debug"
	$ENV{ProgramFiles}/rl/lib
)

FIND_LIBRARY(
	RL_PLAN_LIBRARY_RELEASE
	NAMES
	rlplan
	HINTS
	$ENV{HOME}/workspace/rl/Debug/src/rl/plan
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	$ENV{USERPROFILE}/workspace/rl/Default/src/rl/plan/Release
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/Default/src/rl/plan/Release"
	$ENV{ProgramFiles}/rl/lib
)

IF (RL_PLAN_LIBRARY_DEBUG AND NOT RL_PLAN_LIBRARY_RELEASE)
	SET(RL_PLAN_LIBRARIES ${RL_PLAN_LIBRARY_DEBUG})
ENDIF (RL_PLAN_LIBRARY_DEBUG AND NOT RL_PLAN_LIBRARY_RELEASE)

IF (RL_PLAN_LIBRARY_RELEASE AND NOT RL_PLAN_LIBRARY_DEBUG)
	SET(RL_PLAN_LIBRARIES ${RL_PLAN_LIBRARY_RELEASE})
ENDIF (RL_PLAN_LIBRARY_RELEASE AND NOT RL_PLAN_LIBRARY_DEBUG)

IF (RL_PLAN_LIBRARY_DEBUG AND RL_PLAN_LIBRARY_RELEASE)
	SET(RL_PLAN_LIBRARIES debug ${RL_PLAN_LIBRARY_DEBUG} optimized ${RL_PLAN_LIBRARY_RELEASE})
ENDIF (RL_PLAN_LIBRARY_DEBUG AND RL_PLAN_LIBRARY_RELEASE)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlPlan
	DEFAULT_MSG
	RL_PLAN_INCLUDE_DIRS
	RL_PLAN_LIBRARIES
)

MARK_AS_ADVANCED(
	RL_PLAN_INCLUDE_DIRS
	RL_PLAN_LIBRARIES
	RL_PLAN_LIBRARY_DEBUG
	RL_PLAN_LIBRARY_RELEASE
) 
