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

if HAVE_CA65
check: $(OBJS)
	@set -e; \
	for test in $(SRCS:%.ca65=%); do \
	    echo; \
	    echo "*** $$test: ***"; \
	    opts=$$(sed -n 's/^;#options //p' < "$(srcdir)/$${test}.ca65"); \
	    ( cd $(srcdir) && set -x && $(abs_top_builddir)/src/bobbin --iface simple $$opts --load="$(abs_builddir)/$$test".bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null ); \
	done
else !HAVE_CA65
check:
	@exec >&2; \
	echo; echo '***' No ca65. Skipping opcode tests.; \
	echo
endif !HAVE_CA65

%.bin: %.ca65
	$(CA65) -l $*.lst -o $*.o $<
	$(LD65) $*.o -o $@ -m $*.map -C $(srcdir)/example.cfg
