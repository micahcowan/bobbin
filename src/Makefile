CC=gcc
LD=gcc
CFLAGS=-Wall -Werror -std=c99 -pedantic
OBJECTS=main.o bobbin.o cpu.o mem.o
BOBBIN=bobbin

all: $(BOBBIN)

bobbin: $(OBJECTS)
	$(LD) -o bobbin $(OBJECTS)

%.o: %.c Makefile bobbin-internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(BOBBIN)
