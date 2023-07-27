SRCS = \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65 \
	65C02_extended_opcodes_test.ca65

OBJS = $(SRCS:%.ca65=%.bin)

CA65 = @CA65@
LD65 = @LD65@
EXTRA_DIST = $(SRCS) example.cfg readme.txt
CLEANFILES = $(OBJS) $(SRCS:%.ca65=%.lst) $(SRCS:%.ca65=%.map) $(SRCS:%.ca65=%.o) trace.log

BOBBIN_TARGETS = $(top_builddir)/src/bobbin

all:
check: $(OBJS) $(BOBBIN_TARGETS)
	set -e; \
	if test "$(CA65)" != ":" -a "$(LD65)" != ":"; then \
	    echo '*** 6502 DECIMAL TESTS: ***'; \
	    $(top_builddir)/src/bobbin --iface simple --no-rom --load=6502_decimal_test.bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null; \
	    echo; \
	    echo '*** 6502 FUNCTIONAL TESTS: ***'; \
	    $(top_builddir)/src/bobbin --iface simple --no-rom --load=6502_functional_test.bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null; \
	else \
	    exec 2>&1; \
	    echo; echo '***' No ca65. Skipping opcode tests.; \
	    echo; \
	fi

%.bin: %.ca65
	ca65 -l $*.lst -o $*.o $<
	ld65 $*.o -o $@ -m $*.map -C $(srcdir)/example.cfg

$(BOBBIN_TARGETS):
	$(MAKE) -C $(top_builddir)/src all
