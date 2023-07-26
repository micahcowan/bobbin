noinst_PYTHON = basics.py common.py run_tests.py
BOBBIN_TARGETS = $(top_builddir)/src/bobbin $(top_builddir)/src/fetch-roms
DISTCLEANFILES = $(noinst_PYTHON:.py=.pyc)
all:

distclean-local:
	rm -fr __pycache__

check: $(BOBBIN_TARGETS)
	if test '$(PYTHON)' != : ; then \
	    PYTHONPATH=$(srcdir) $(PYTHON) $(srcdir)/run_tests.py $(CHECK_VERBOSE); \
	else \
	    exec >&2; \
	    echo; echo '***' No python 3, so SKIPPING pexpect tests; echo; \
	fi

$(BOBBIN_TARGETS):
	$(MAKE) -C $(top_builddir)/src all

run-bobbin:
	BOBBIN_ROMDIR=$(top_builddir)/src/roms $(top_builddir)/src/bobbin $(ARGS)
