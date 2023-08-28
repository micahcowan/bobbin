AUTOMAKE_OPTIONS=subdir-objects
WARNINGS=-Wall -Werror -Wno-unused -Wno-unused-result
# XXX: ROMSRCHDIR should be decided in ./configure script
romdir=$(pkgdatadir)/roms
cardromdir=$(romdir)/cards
AM_CPPFLAGS=-I$(PWD) -DROMSRCHDIR='"$(romdir)"'
#CCDEBUG=-g -Og
CFLAGS=$(WARNINGS) -std=c99 -pedantic $(CCDEBUG)
bobbin_SOURCES=main.c bobbin.c config.c cpu.c mem.c trace.c interfaces/iface.c interfaces/simple.c util.c signal.c debug.c disasm.c machine.c event.c hook.c watch.c cmd.c periph.c periph/disk2.c format.c format/nib.c format/dsk.c format/empty.c sha-256.c sha-256.h bobbin-internal.h apple2.h ac-config.h
bobbin_LDADD=$(BOBBIN_MAYBE_TTY) $(LIBCURSES)
bobbin_DEPENDENCIES=$(BOBBIN_MAYBE_TTY)
EXTRA_bobbin_SOURCES=interfaces/tty.c
sha256_verify_SOURCES=sha256-verify.c sha-256.c
bin_PROGRAMS=bobbin
noinst_PROGRAMS=sha256-verify
BUILT_SOURCES = option-names.h machine-names.h help-text.h
EXTRA_DIST = scripts/gen-help.awk scripts/gen-options.awk \
	     scripts/gen-machines.awk scripts/check-options.awk \
	     $(BUILT_SOURCES)
NO_LICENSE_OK_FILES = sha-256.c sha-256.h apple2.h ac-config.h
A2ROMS=roms/apple2.rom roms/apple2plus.rom
dist_rom_DATA=$(A2ROMS)
dist_cardrom_DATA=roms/cards/disk2.rom
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

check: ck-options-impl ck-license
	@missing=; \
	for file in $(bobbin_SOURCES) $(EXTRA_bobbin_SOURCES) scripts/*.awk; do \
	    if ! head -n 10 $$file | grep -q 'This code is licensed under the MIT license'; then \
	        case $$file in \
	            sha-256.c|sha-256.h|apple2.h|ac-config.h) \
	                ;; \
	            *) \
	                missing="$$missing $$file"; \
	                ;; \
	        esac; \
	    fi; \
	done; \
	if test "x$$missing" != x; then \
	    echo "*** The following files need license info:"; \
	    for file in $$missing; do \
	        echo $$file; \
	    done; \
	    exit 1; \
	fi

.PHONY: ck-options-impl
ck-options-impl: option-names.h $(srcdir)/config.c
	cat option-names.h $(srcdir)/config.c | $(AWK) -f $(srcdir)/scripts/check-options.awk

.PHONY: ck-license
ck-license:

option-names.h: ../README.md scripts/gen-options.awk
	$(AWK) -f $(srcdir)/scripts/gen-options.awk < $< > $@.out
	mv $@.out $@

help-text.h: ../README.md scripts/gen-help.awk
	$(AWK) -f $(srcdir)/scripts/gen-help.awk -v DOCDIR=$(docdir) < $< > $@.out
	mv $@.out $@

machine-names.h: ../README.md scripts/gen-machines.awk
	$(AWK) -f $(srcdir)/scripts/gen-machines.awk < $< > $@.out
	mv $@.out $@
