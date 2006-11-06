#
# Mac-on-Linux makefile
# Copyright (C) 2004 Samuel Rydh (samuel@ibrium.se)
#

top-all:	
include		config/Makefile.top
include		config/Makefile.master

SUBDIRS		= . scripts src util/img
MAKE		+= -s

$(OINC)/cpu: $(OINC)/.dir
	@rm -f $@
	@ln -s ../stree/src/include/$(CPU) $@

$(OINC)/asm: $(OINC)/.dir
	@rm -f $@
	@ln -s ../stree/src/include/$(ARCH) $@

$(OINC)/kern/kconfig.h: $(OINC)/autoconf.h
	@rm -f $@
	@test -d $(dir $@) || $(INSTALL) -d $(dir $@)
	@cat $< | grep CONFIG_ | sed s/CONFIG/CONFIG_MOL/g > $@

local-includes: $(OINC)/asm $(OINC)/cpu $(OINC)/kern/kconfig.h
modules: bootstrap

#####################################################################

help:
	@printf "\nCONFIGURING:\n"
	@printf "  %-30s - %s\n" "make menuconfig" "change MOL settings"
	@printf "\nBUILDING:\n"
	@printf "  %-30s - %s\n" "make" "build MOL"
	@printf "  %-30s - %s\n" "make configure" "rerun configure script"
	@printf "  %-30s - %s\n" "make [dist]clean" "remove build products"
	@printf "  %-30s - %s\n" "make modules" "build only the kernel module(s)"
	@printf "  %-30s - %s\n" "./autogen.sh" "regenerate build files"
	@printf "\nINSTALL:\n"
	@printf "  %-30s - %s\n" "make install" "install MOL"
	@printf "  %-30s - %s\n" "make install-config" "install default config files"
	@printf "  %-30s - %s\n" "make install-modules" "install kernel modules (and nothing else)"
	@printf "  %-30s - %s\n" "make uninstall" "uninstall MOL"
	@printf "  %-30s - %s\n" "make uninstall-all" "uninstall everything (including config files)"
	@printf "  %-30s - %s\n" "make paths" "show install paths"
	@printf "\nBITKEEPER/RSYNC support:\n"
	@printf "  %-30s - %s\n" "make libimport" "download binary support files"
	@printf "  %-30s - %s\n" "make libpopulate" "use binary suport files in ./libimport"
	@printf "\nDISTRIBUTION:\n"
	@printf "  %-30s - %s\n" "make dist" "create tar archive"
	@printf "  %-30s - %s\n" "make rpms" "build all RPMs"
	@printf "  %-30s - %s\n" "make rpm" "build mol RPM"
	@printf "  %-30s - %s\n" "make kmods_rpm" "build kernel module RPM"
	@echo

#####################################################################

libimport:
	scripts/libimport import
libpopulate:
	scripts/libimport populate
libimport_dist:
	scripts/libimport import_dist
libdelete:
	scripts/libimport delete
libexport: clean
	scripts/libimport export
libimport_clean:
	$(RM) -rf libimport

.PHONY: libimport libpopulate libimport_dist libdelete libexport libimport_clean


#####################################################################

clean-local:
	@rm -rf $(mollib)/bin $(top_odir)/deps $(top_odir)/util $(top_odir)/src
distclean-local:
	@rm -rf mollib/modules

#####################################################################

include		$(rules)/Rules.make
include		Makefile.dist
