AUTOMAKE_OPTIONS = foreign

TESTSUITE_TIF = $(srcdir)/testsuite_tif

if OPENJPEG_FOUND
TESTSUITE_JP2 = $(srcdir)/testsuite_jp2
else
TESTSUITE_JP2 = 
endif

EXTRA_DIST = $(TESTSUITE_TIF).at $(TESTSUITE_TIF) \
package.m4.in $(srcdir)/package.m4 atlocal.in bes.conf.in \
bes.conf.modules.in gdal

if OPENJPEG_FOUND
EXTRA_DIST += $(TESTSUITE_JP2).at $(TESTSUITE_JP2)
endif

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

$(TESTSUITE_TIF): $(TESTSUITE_TIF).at $(srcdir)/package.m4 bes.conf
	$(AUTOTEST) -I '$(srcdir)' -o ${srcdir}/$@.tmp $@.at
	mv ${srcdir}/$@.tmp ${srcdir}/$@

$(TESTSUITE_JP2): $(TESTSUITE_JP2).at $(srcdir)/package.m4 bes.conf
	$(AUTOTEST) -I '$(srcdir)' -o ${srcdir}/$@.tmp $@.at
	mv ${srcdir}/$@.tmp ${srcdir}/$@
