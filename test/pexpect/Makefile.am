noinst_PYTHON = basics.py asoft.py common.py run_tests.py
DISTCLEANFILES = $(noinst_PYTHON:.py=.pyc)
all:

distclean-local:
	rm -fr __pycache__

check:
	if test 'x$(PYTHON)' != x: -a 'x$(PYTHON)' != x; then \
	    BOBBIN_ROMDIR=$(top_srcdir)/src/roms PYTHONPATH=$(srcdir) $(PYTHON) $(srcdir)/run_tests.py $(CHECK_VERBOSE); \
	else \
	    exec >&2; \
	    echo; echo '***' No python 3, so SKIPPING pexpect tests; echo; \
	fi

run-bobbin:
	$(top_builddir)/src/bobbin $(ARGS)
