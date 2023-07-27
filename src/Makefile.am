AUTOMAKE_OPTIONS=subdir-objects
WARNINGS=-Wall -Werror -Wno-unused -Wno-unused-result
# XXX: ROMSRCHDIR should be decided in ./configure script
ROMSRCHDIR=$(datarootdir)/$(PACKAGE_TARNAME)
AM_CPPFLAGS=-I$(PWD) -DROMSRCHDIR='"$(ROMSRCHDIR)"'
#CCDEBUG=-g -Og
CFLAGS=$(WARNINGS) -std=c99 -pedantic $(CCDEBUG)
bobbin_SOURCES=main.c bobbin.c config.c cpu.c mem.c trace.c interfaces/iface.c interfaces/simple.c util.c signal.c debug.c disasm.c machine.c hook.c sha-256.c sha-256.h bobbin-internal.h apple2.h
sha256_verify_SOURCES=sha256-verify.c sha-256.c
bin_PROGRAMS=bobbin
noinst_PROGRAMS=sha256-verify
BUILT_SOURCES = option-names.h machine-names.h help-text.h
EXTRA_DIST = scripts/gen-help.awk scripts/gen-options.awk \
	     scripts/gen-machines.awk scripts/check-options.awk \
	     roms/apple2.rom roms/apple2plus.rom
CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = roms/*.rom cscope.out trace.log

.PHONY: watch
watch:
	@echo Watching...
	@while : ; do \
	    if $(MAKE) ; then \
	        printf '\033[1;32mSUCCESS\033[m\n'; \
	    else \
	        printf '\033[1;31mFAILURE\033[m\n'; \
	    fi; \
	    echo; \
	    echo; \
	    inotifywait -e modify -r . ; \
	done

check: ck-options-impl

.PHONY: ck-options-impl
ck-options-impl: option-names.h $(srcdir)/config.c
	cat option-names.h $(srcdir)/config.c | $(AWK) -f $(srcdir)/scripts/check-options.awk

option-names.h: ../README.md scripts/gen-options.awk
	$(AWK) -f $(srcdir)/scripts/gen-options.awk < $< > $@.out
	mv $@.out $@

help-text.h: ../README.md scripts/gen-help.awk
	$(AWK) -f $(srcdir)/scripts/gen-help.awk < $< > $@.out
	mv $@.out $@

machine-names.h: ../README.md scripts/gen-machines.awk
	$(AWK) -f $(srcdir)/scripts/gen-machines.awk < $< > $@.out
	mv $@.out $@
