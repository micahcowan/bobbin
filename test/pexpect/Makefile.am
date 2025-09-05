noinst_PYTHON = basics.py debug.py asoft.py common.py run_tests.py \
                apple_iie.py illegal_ops.py
DISTCLEANFILES = $(noinst_PYTHON:.py=.pyc)
all:

distclean-local:
	rm -fr __pycache__

if HAVE_PEXPECT
check:
	BOBBIN_ROMDIR=$(top_srcdir)/src/roms PYTHONPATH=$(srcdir) $(PYTHON) $(srcdir)/run_tests.py $(CHECK_VERBOSE)
else !HAVE_PEXPECT
check:
	@exec >&2; \
	echo; echo '***' No python 3, so SKIPPING pexpect tests; echo
endif !HAVE_PEXPECT

run-bobbin:
	$(top_builddir)/src/bobbin $(ARGS)
