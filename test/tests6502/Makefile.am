SRCS = \
	language_card.ca65 \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65

# 65C02 tests - only run on Enhanced Apple IIe
SRCS_65C02 = \
	65c02_stack_test.ca65 \
	65c02_bra_test.ca65 \
	65c02_stz_test.ca65 \
	enhanced_iie_test.ca65

OBJS = $(SRCS:%.ca65=%.bin)
OBJS_65C02 = $(SRCS_65C02:%.ca65=%.bin)

CA65 = @CA65@
LD65 = @LD65@
EXTRA_DIST = $(SRCS) $(SRCS_65C02) 65c02_test_common.inc example.cfg readme.txt test.rom
CLEANFILES = $(OBJS) $(OBJS_65C02) $(SRCS:%.ca65=%.lst) $(SRCS:%.ca65=%.map) $(SRCS:%.ca65=%.o) $(SRCS_65C02:%.ca65=%.lst) $(SRCS_65C02:%.ca65=%.map) $(SRCS_65C02:%.ca65=%.o) trace.log

all:

if HAVE_CA65
check: $(OBJS) $(OBJS_65C02)
	@set -e; \
	echo; echo "*** Running 6502 CPU tests ***"; \
	for test in $(SRCS:%.ca65=%); do \
	    echo; \
	    echo "*** $$test: ***"; \
	    opts=$$(sed -n 's/^;#options //p' < "$(srcdir)/$${test}.ca65"); \
	    ( cd $(srcdir) && set -x && $(abs_top_builddir)/src/bobbin --iface simple $$opts --load="$(abs_builddir)/$$test".bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null ); \
	done; \
	echo; echo "*** Running 65C02 CPU tests on Enhanced Apple IIe ***"; \
	for test in $(SRCS_65C02:%.ca65=%); do \
	    echo; \
	    echo "*** $$test (Enhanced Apple IIe): ***"; \
	    opts=$$(sed -n 's/^;#options //p' < "$(srcdir)/$${test}.ca65"); \
	    ( cd $(srcdir) && set -x && $(abs_top_builddir)/src/bobbin -m enhanced --iface simple $$opts --load="$(abs_builddir)/$$test".bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null ); \
	done
else !HAVE_CA65
check:
	@exec >&2; \
	echo; echo '***' No ca65. Skipping opcode tests.; \
	echo
endif !HAVE_CA65

# 65C02 tests depend on the common header
65c02_stack_test.bin: 65c02_test_common.inc
65c02_bra_test.bin: 65c02_test_common.inc
65c02_stz_test.bin: 65c02_test_common.inc

%.bin: %.ca65
	$(CA65) -l $*.lst -o $*.o $<
	$(LD65) $*.o -o $@ -m $*.map -C $(srcdir)/example.cfg
