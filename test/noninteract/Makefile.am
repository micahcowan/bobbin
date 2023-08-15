EXTRA_DIST = $(wildcard *.t/run) $(wildcard *.t/input) $(wildcard *.t/exstat) $(wildcard *.t/expected) $(wildcard *.t/indisk*)
CLEANFILES = *.t/output *.t/testdisk.*
BTESTS = $(notdir $(wildcard $(srcdir)/*.t) )
check:
	export TESTDIR=$(abs_srcdir); \
	export BOBBIN=$(abs_top_srcdir)/src/bobbin; \
	export DISKS=$(abs_top_srcdir)/disk; \
	sh run_tests.sh $(BTESTS)
