SRCS_6502 = \
	language_card.ca65 \
	6502_decimal_test.ca65 \
	6502_functional_test.ca65

# 65C02 tests - only run on Enhanced Apple IIe
SRCS_65C02 = \
	65C02_extended_opcodes_test.ca65 \
	6502_functional_test.ca65 \
	65c02_decimal_test.ca65 \
	65c02_stack_test.ca65 \
	65c02_bra_test.ca65 \
	65c02_stz_test.ca65 \
	enhanced_iie_test.ca65

SRCS = $(SRCS_6502) $(SRCS_65C02)

OBJS_6502 = $(SRCS:%.ca65=%.bin)
OBJS_65C02 = $(SRCS_65C02:%.ca65=%.bin)
OBJS = $(OBJS_6502) $(OBJS_65C02)

CA65 = @CA65@
LD65 = @LD65@
EXTRA_DIST = $(SRCS) decimal_test_common.inc 65c02_test_common.inc example.cfg readme.txt test.rom
CLEANFILES = $(OBJS) $(SRCS:%.ca65=%.lst) $(SRCS:%.ca65=%.map) $(SRCS:%.ca65=%.o)

all:

if HAVE_CA65
check: $(OBJS)
	@set -e; \
	pass=0; fail=0; xfail=0; \
	run_test() { \
	    local test="$$1"; shift; \
	    local opts="$$(sed -n 's/^;#options //p' < "$(srcdir)/$${test}.ca65")"; \
	    echo; \
	    if grep -q '^;#EXPECT-FAILURE$$' $(srcdir)/$${test}.ca65; then \
	        xxx=true; \
	        echo "*** $$test (EXPECTING FAILURE): ***"; \
	    else \
	        xxx=''; \
	        echo "*** $$test: ***"; \
	    fi; \
	    if ( cd $(srcdir) && set -x && $(abs_top_builddir)/src/bobbin --iface simple $$xopts $$opts --load="$(abs_builddir)/$$test".bin --trap-failure 0x0001 --trap-success 0x0002 </dev/null ); then \
	        if test "x$${xxx}" != x; then \
	            echo '  (unexpected pass, treating as fail)'; \
	            fail=$$(( $$fail + 1 )); \
	        else \
	            pass=$$(( $$pass + 1 )); \
	        fi; \
	    elif test "x$${xxx}" != x; then \
	        xfail=$$(( $$xfail + 1 )); \
	        echo '  (expected fail)'; \
	    else \
	        fail=$$(( $$fail + 1 )); \
	    fi; \
	}; \
	xopts='-m plus'; \
	echo; echo "*** Running 6502 CPU tests on Apple ][+ ***"; \
	for test in $(SRCS_6502:%.ca65=%); do \
	    run_test "$$test"; \
	done; \
	echo; echo "*** Running 65C02 CPU tests on Enhanced Apple IIe ***"; \
	xopts='-m enhanced'; \
	for test in $(SRCS_65C02:%.ca65=%); do \
	    run_test "$$test"; \
	done; \
	echo; \
	echo 'Test results summary, ca65:'; echo; \
	echo "    $$pass passed, $$fail failed, $$xfail expected fails."; \
	if test "$$fail" -ne 0; then \
	    echo 'One or more tests FAILED. Exiting with failure status.'; \
	    exit 1; \
	fi;
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
