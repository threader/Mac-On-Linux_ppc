
include			../../config/Makefile.top

INCLUDES		= -I ../include

CFLAGS			+= -fno-builtin -msoft-float $(NOSTACKPROTECT)

XTARGETS		= accel
accel-OBJS		= mol.o molaccel.o 

include			$(rules)/Rules.make
