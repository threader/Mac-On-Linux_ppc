
include		../../config/Makefile.top

INCLUDES	= -I ../include

CFLAGS		+= -fno-builtin -msoft-float $(NOSTACKPROTECT)

XTARGETS	= libc
libc-OBJS	= mem.o string.o zalloc.o vsprintf.o strtol.o

include		$(rules)/Rules.make
