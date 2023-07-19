CC=gcc
LD=gcc
WARNINGS=-Wall -Werror -Wno-unused -Wno-unused-result
CPPFLAGS=-I$(PWD)
CFLAGS=$(WARNINGS) -std=c99 -pedantic -g -Og
OBJECTS=main.o bobbin.o config.o cpu.o mem.o trace.o interfaces/iface.o interfaces/pipe.o interfaces/simple.o util.o signal.o debug.o disasm.o machine.o
HEADERS=$(wildcard *.h)
BOBBIN=bobbin

.PHONY: all
all: $(BOBBIN)

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
	BOBBIN_TEST=6502_functional_test.bin ./bobbin

6502_functional_test.bin:
	$(MAKE) -C ../test/tests6502/
	cp  ../test/tests6502/$@ ./$@

test: check

bobbin: $(OBJECTS)
	$(LD) -o bobbin $(OBJECTS)

%.o: %.c Makefile $(HEADERS)
	cd $(dir $@) && $(CC) $(CPPFLAGS) $(CFLAGS) -c $(notdir $<)

.PHONY: clean
clean:
	rm -f *.o $(BOBBIN)
