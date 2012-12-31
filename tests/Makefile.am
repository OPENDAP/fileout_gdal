AUTOMAKE_OPTIONS = foreign

TESTSUITE_TIF = $(srcdir)/testsuite_tif

if OPENJPEG_FOUND
TESTSUITE_JP2 = $(srcdir)/testsuite_jp2
else
TESTSUITE_JP2 = 
endif

EXTRA_DIST = $(TESTSUITE_TIF).at $(TESTSUITE_TIF) $(TESTSUITE_JP2).at $(TESTSUITE_JP2) \
	package.m4.in $(srcdir)/package.m4 atlocal.in template.bescmd.in bes.conf.in gdal

noinst_DATA = bes.conf
DISTCLEANFILES = bes.conf

check-local: atconfig atlocal $(TESTSUITE_TIF) $(TESTSUITE_JP2)
	$(SHELL) '$(TESTSUITE_TIF)' $(TESTSUITE_TIFFLAGS)
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' $(TESTSUITE_TIFFLAGS)

clean-local:
	test ! -f '$(TESTSUITE_TIF)' || $(SHELL) '$(TESTSUITE_TIF)' --clean
	test ! -f '$(TESTSUITE_JP2)' || $(SHELL) '$(TESTSUITE_JP2)' --clean

distclean-local:
	-rm atconfig
	-rm bes.conf


# Use the missing script for better error checking on autom4te run.
AUTOM4TE = $(SHELL) $(top_srcdir)/conf/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

$(TESTSUITE_TIF): $(srcdir)/testsuite_tif.at $(srcdir)/package.m4 bes.conf
	$(AUTOTEST) -I '$(srcdir)' -o ${srcdir}/$@.tmp $@.at
	mv ${srcdir}/$@.tmp ${srcdir}/$@

bes.conf: bes.conf.in ../config.status
	sed -e "s%[@]pkgdatadir[@]%${pkgdatadir}%" \
		-e "s%[@]sysconfdir[@]%${sysconfdir}%" \
		-e "s%[@]beslibdir[@]%${BES_MODULE_DIR}%" \
		-e "s%[@]datadir[@]%${datadir}%" \
		-e "s%[@]abs_top_srcdir[@]%${abs_top_srcdir}%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" \
		-e "s%[@]bindir[@]%${bindir}%" $< > bes.conf


