
include			../../config/Makefile.top

SUBDIRS-$(MPC107)	= mpc107
SUBDIRS-$(PPC)		= ppc
SUBDIRS-$(X86)		= i386

XTARGETS		= cpu
#CFLAGS			+= -msoft-float

cpu-OBJS		= mainloop.o molcpu.o $(obj-y)
obj-$(CONFIG_DEBUGGER)	+= breakpoints.o
obj-$(PPC)		+= ppc/libppc.a
obj-$(X86)		+= i386/libi386.a
obj-$(MPC107)		+= ppc/libmpc107.a


include			$(rules)/Rules.make
