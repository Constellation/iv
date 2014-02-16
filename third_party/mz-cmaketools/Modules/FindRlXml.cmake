FIND_PATH(
	RL_XML_INCLUDE_DIRS
	NAMES
	rl/xml/Node.h
	HINTS
	$ENV{HOME}/workspace/rl/src
	$ENV{HOME}/include
	/usr/local/include
	/usr/include
	$ENV{USERPROFILE}/workspace/rl/src
	"[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders;Personal]/workspace/rl/src"
	$ENV{ProgramFiles}/rl/include
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	RlXml
	DEFAULT_MSG
	RL_XML_INCLUDE_DIRS
)

MARK_AS_ADVANCED(
	RL_XML_INCLUDE_DIRS
) 
