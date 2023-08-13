BOBBIN_TARGETS = $(top_builddir)/src/bobbin
SUBDIRS = pexpect tests6502

check-recursive: $(BOBBIN_TARGETS)

$(BOBBIN_TARGETS):
	$(MAKE) -C $(top_builddir)/src all
