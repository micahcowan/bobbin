SRCS = \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65 \
	65C02_extended_opcodes_test.ca65

OBJS = $(SRCS:%.ca65=%.bin)

CA65 = @CA65@
LD65 = @LD65@
EXTRA_DIST = $(SRCS)
CLEANFILES = $(OBJS) $(SRCS:%.ca65=%.lst) $(SRCS:%.ca65=%.map) $(SRCS:%.ca65=%.o) trace.log

all:
check: $(OBJS)
	if test "$(CA65)" != ":" -a "$(LD65)" != ":"; then \
	    BOBBIN_TEST=6502_functional_test.bin BOBBIN_ROMDIR=$(top_builddir)/src/roms $(top_builddir)/src/bobbin -m ][+ --iface simple; \
	else \
	    exec 2>&1; \
	    echo; echo '***' No ca65. Skipping opcode tests.; \
	    echo; \
	fi

%.bin: %.ca65
	ca65 -l $*.lst -o $*.o $<
	ld65 $*.o -o $@ -m $*.map -C $(srcdir)/example.cfg

