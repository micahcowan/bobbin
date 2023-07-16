SRCS = \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65 \
	65C02_extended_opcodes_test.ca65

OBJS = $(SRCS:%.ca65=%.bin)

all: $(OBJS)

%.bin: %.ca65
	ca65 -l $*.lst $<
	ld65 $*.o -o $@ -m $*.map -C example.cfg

