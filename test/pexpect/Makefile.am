noinst_PYTHON = basics.py common.py run_tests.py

check:
	if test '$(PYTHON)' != : ; then \
	    PYTHONPATH=$(srcdir) $(PYTHON) $(srcdir)/run_tests.py; \
	else \
	    exec >&2; \
	    echo; echo '***' No python 3, so SKIPPING pexpect tests; echo; \
	fi
