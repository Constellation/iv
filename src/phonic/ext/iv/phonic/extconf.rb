require 'mkmf'
$CFLAGS += " -Wall -Werror -Wno-unused-parameter "
dir_config('iv', 'ext')
$CFLAGS += " -I../../include "

create_makefile('iv/phonic')
