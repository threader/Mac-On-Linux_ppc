
include			../../../config/Makefile.top

XTARGETS		= sound
sound-OBJS		= $(obj-y)

INCLUDES		= -I../include

obj-$(CONFIG_ALSA)	+= alsa.o
obj-$(CONFIG_OSS)	+= oss.o
obj-y			+= sound.o

include			$(rules)/Rules.make
