
include			../rules/Makefile.top


SUBDIRS 		= fs libc main accel .
XTARGETS		= bootx

bootx-OBJS		= fs/$(ODIR)/libfs.a libc/$(ODIR)/liblibc.a main/$(ODIR)/libmain.a accel/$(ODIR)/libaccel.a

# WARNING: We may only put things at 0x1c00000 - 0x1d00000.
# We are very close to the end limit.

.dummy.c: Makefile
	rm -f $@
	echo "extern void _start(void);" > $@
	echo "extern int main( void ); int main( void ) { _start(); return 0; }" >> $@


bootx: Makefile $(ODIR)/libbootx.a $(ODIR)/.dummy.o
	$(LD) -Ttext=0x1c01000 $(ODIR)/.dummy.o -Bstatic $(ODIR)/libbootx.a $(LIBGCC) -o $@ 
	nm $@ | sort > bootx.syms
	$(STRIP) $@
	cp bootx ../../mollib/drivers/bootx

all-local: bootx

include			$(rules)/Rules.make
