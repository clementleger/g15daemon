LLIBDIR=@LIBDIR@


SUBDIRS =  libg15daemon_client g15daemon
AM_CPPFLAGS =  -I$(top_srcdir)/libg15daemon_client -I$(top_srcdir)/g15daemon
EXTRA_DIST = debian contrib Documentation g15daemon_xmms lang-bindings patches rpm README.usage FAQ LICENSE

docdir = $(prefix)/share/doc/$(PACKAGE)-$(VERSION)
doc_DATA = FAQ README.usage README ChangeLog TODO AUTHORS NEWS LICENSE

man1_MANS = $(top_srcdir)/Documentation/g15daemon.1
man3_MANS = $(top_srcdir)/Documentation/g15daemon_client_devel.3

install-data-hook:
if KEXT_INSTALL
	 cp -Rf $(top_srcdir)/contrib/libusbshield.kext @KEXT_LOCATION@
	 chown -R root:wheel @KEXT_LOCATION@/libusbshield.kext
endif

dist-hook:
	rm -rf `find $(distdir)/debian -name .svn`
	rm -rf `find $(distdir)/contrib -name .svn`
	rm -rf `find $(distdir)/Documentation -name .svn`
	rm -rf `find $(distdir)/g15daemon_xmms -name .svn`
	rm -rf `find $(distdir)/lang-bindings -name .svn`
	rm -rf `find $(distdir)/patches -name .svn`
	rm -rf `find $(distdir)/rpm -name .svn`

dist-rpm: dist-bzip2
	rpmbuild -ts $(distdir).tar.bz2

dist-archives:
	$(MAKE) dist-bzip2
	$(MAKE) dist-rpm                
