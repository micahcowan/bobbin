SRCS = \
        language_card.ca65 \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65

OBJS = $(SRCS:%.ca65=%.bin)

CA65 = @CA65@
LD65 = @LD65@
EXTRA_DIST = $(SRCS) example.cfg readme.txt 65C02_extended_opcodes_test.ca65
CLEANFILES = $(OBJS) $(SRCS:%.ca65=%.lst) $(SRCS:%.ca65=%.map) $(SRCS:%.ca65=%.o) trace.log

all:
check: $(OBJS)
	set -e; \
	if test "$(CA65)" != ":" -a "$(LD65)" != ":"; then \
	    for test in $(SRCS:%.ca65=%); do \
		echo; \
		echo "*** $$test: ***"; \
		opts=$$(sed -n 's/^;#options //p' < "$(srcdir)/$${test}.ca65"); \
		( cd $(srcdir) && set -x && $(abs_top_builddir)/src/bobbin --iface simple $$opts --load="$(abs_builddir)/$$test".bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null ); \
	    done; \
	else \
	    exec 2>&1; \
	    echo; echo '***' No ca65. Skipping opcode tests.; \
	    echo; \
	fi

%.bin: %.ca65
	ca65 -l $*.lst -o $*.o $<
	ld65 $*.o -o $@ -m $*.map -C $(srcdir)/example.cfg
