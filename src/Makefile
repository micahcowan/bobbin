CC=gcc
LD=gcc
CFLAGS=-Wall -Werror -std=c99 -pedantic -Wno-unused
OBJECTS=main.o bobbin.o cpu.o mem.o trace.o iface-simple.o util.o signal.o debug.o
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
	    inotifywait -e modify . ; \
	done

bobbin: $(OBJECTS)
	$(LD) -o bobbin $(OBJECTS)

%.o: %.c Makefile $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(BOBBIN)
