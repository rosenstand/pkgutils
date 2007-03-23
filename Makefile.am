SUBDIRS = etc include man scripts src

install-data-local:
	$(INSTALL) -d $(DESTDIR)$(localstatedir)/lib
	$(INSTALL) -m 0751 -d $(DESTDIR)$(localstatedir)/lib/pkg

GITVERSION = `git log -1 --pretty=oneline | cut -b 0-8`

dist_git:
	make dist VERSION=${PACKAGE_VERSION}-${GITVERSION}
