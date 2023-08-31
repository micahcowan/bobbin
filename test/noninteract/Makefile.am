EXTRA_DIST = run_tests.sh $(wildcard *.t/run) $(wildcard *.t/input) $(wildcard *.t/exstat) $(wildcard *.t/expected) $(wildcard *.t/indisk*)
CLEANFILES = *.t/output *.t/testdisk.*
BTESTS = $(notdir $(wildcard $(srcdir)/*.t) )
check:
	export TESTDIR=$(abs_srcdir); \
	export BOBBIN=$(abs_top_builddir)/src/bobbin; \
	export BOBBIN_ROMDIR=$(abs_top_srcdir)/src/roms; \ 
	export DISKS=$(abs_top_srcdir)/disk; \
	sh $(srcdir)/run_tests.sh $(BTESTS)
