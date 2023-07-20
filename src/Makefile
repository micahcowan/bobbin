CC=gcc
LD=gcc
WARNINGS=-Wall -Werror -Wno-unused -Wno-unused-result
CPPFLAGS=-I$(PWD) -DPREFIX='"$(PREFIX)"'
#CCDEBUG=-g -Og
CFLAGS=$(WARNINGS) -std=c99 -pedantic $(CCDEBUG)
BOBBIN-OBJECTS=main.o bobbin.o config.o cpu.o mem.o trace.o interfaces/iface.o interfaces/simple.o util.o signal.o debug.o disasm.o machine.o sha-256.o
SHAVERIFY-OBJECTS=sha256-verify.o sha-256.o
HEADERS=$(wildcard *.h)
BOBBIN=bobbin
SHAVERIFY=sha256-verify

# Can be overridden on cmdline:
PREFIX=/usr

.PHONY: all
all: $(BOBBIN) fetch-roms

.PRECIOUS: roms
roms:
	ln -sf ../roms

.PHONY: fetch-roms
fetch-roms: $(SHAVERIFY) roms
	cd roms/ && ./fetch-roms.sh; true

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

.PHONY: check test
check: $(BOBBIN) 6502_functional_test.bin
	BOBBIN_TEST=6502_functional_test.bin ./bobbin -m ][+ --iface simple

6502_functional_test.bin:
	$(MAKE) -C ../test/tests6502/
	cp  ../test/tests6502/$@ ./$@

test: check

$(SHAVERIFY): $(SHAVERIFY-OBJECTS)
	$(LD) -o $(SHAVERIFY) $(SHAVERIFY-OBJECTS)

$(BOBBIN): $(BOBBIN-OBJECTS)
	$(LD) -o $(BOBBIN) $(BOBBIN-OBJECTS)

%.o: %.c Makefile $(HEADERS)
	cd $(dir $@) && $(CC) $(CPPFLAGS) $(CFLAGS) -c $(notdir $<)

.PHONY: clean
clean:
	rm -f *.o */*.o $(BOBBIN) 6502_functional_test.bin $(SHAVERIFY)
