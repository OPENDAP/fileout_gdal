# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([fileout_gdal], [0.10.0], [opendap-tech@opendap.org])
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE(1.10)

AM_CONDITIONAL([DAP_MODULES], [false])

# The handler is a shared object library, so we don't normally need
# the static lib. Override this default by specifying `--enable-static',
# which can make running unit tests with valgrind far more pleasant. 
AM_DISABLE_STATIC

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

AC_CONFIG_SRCDIR([FONgTransform.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

# Support for large files?
AC_SYS_LARGEFILE

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.13.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
 
AC_CHECK_BES([3.13.0],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])

# Which copy of GDAL should be used for the build?
GDAL_FOUND=

AC_ARG_WITH(gdal, AS_HELP_STRING([--with-gdal], [Use the copy of GDAL at this location]),
            with_gdal_prefix="$withval", with_gdal_prefix="")

if test -n "$with_gdal_prefix" -a -x $with_gdal_prefix/bin/gdal-config
then
    AC_MSG_NOTICE([Using $with_gdal_prefix as the GDAL prefix directory.])
    GDAL_LDFLAGS="`$with_gdal_prefix/bin/gdal-config --libs` `$with_gdal_prefix/bin/gdal-config --dep-libs`"
    GDAL_CFLAGS="`$with_gdal_prefix/bin/gdal-config --cflags`"
    GDAL_FOUND="yes"
    
    AC_MSG_NOTICE([Set GDAL_LDFLAGS to $GDAL_LDFLAGS.])
elif test -n "$with_gdal_prefix"
then
    AC_MSG_ERROR([You set the gdal-prefix directory to $with_gdal_prefix, but gdal-config is not there.])
fi

if test -z "$GDAL_FOUND"
then
    AC_MSG_NOTICE([Looking for GDAL.])
    AX_LIB_GDAL([1.10.0])
    if test ! -z "$GDAL_CFLAGS" -a ! -z "$GDAL_LDFLAGS"; then
        GDAL_FOUND="yes"
    fi
fi

if test -z "$GDAL_FOUND"
then
    AC_MSG_ERROR([I could not find GDAL.])
fi

# Which copy of OpenJPEG should be used for the build?
OPENJPEG_FOUND=

AC_ARG_WITH(openjpeg, AS_HELP_STRING([--with-openjpeg], [Use the copy of openjpeg at this location]),
            with_openjpeg_prefix="$withval", with_openjpeg_prefix="")

if test -n "$with_openjpeg_prefix" -a -d "$with_openjpeg_prefix/lib"
then
    AC_MSG_NOTICE([Using $with_openjpeg_prefix as the openjpeg prefix directory.])
    OPENJPEG_LDFLAGS="-L$with_openjpeg_prefix/lib -lopenjp2"
    OPENJPEG_FOUND="yes"
elif test -n "$with_openjpeg_prefix"
then
    AC_MSG_ERROR([You set the openjpeg-prefix directory to $with_openjpeg_prefix, but there is no 'lib' directory there.])
fi

AM_CONDITIONAL([OPENJPEG_FOUND], [test "x$OPENJPEG_FOUND" = "xyes"])

if test -z "$OPENJPEG_FOUND"
then
  AC_MSG_NOTICE([I could not find OPENJPEG, not building GMLJP2 support.])
else
  AC_MSG_NOTICE([Found OPENJPEG; building GMLJP2 support.])
fi

AC_ARG_ENABLE([developer],
[AS_HELP_STRING([--enable-developer],
[Build a debug (-g3 -O0) version of this code (default is no)])],
[case "${enableval}" in
       yes) build_hyrax_debug=true ;;
       no)  build_hyrax_debug=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
     esac],
[build_hyrax_debug=false])

if test "x$build_hyrax_debug" = "xtrue"; then
   AC_MSG_NOTICE(["Building debug version"])
else
   AC_MSG_NOTICE(["Not building debug version"])
   AC_DEFINE([NDEBUG], [1], [Remove this to activate assert() calls.])
fi
AM_CONDITIONAL([BUILD_HYRAX_DEBUG], [test "x$build_hyrax_debug" = "xtrue"])


## This sets up the special symbols used to build the bes.conf file for
## the tests. Previously, this was done in the Makefile.am down in the
## test directory, but building the bes.conf here works better with
## autoconf overall and esp. with the 'modules' build. jhrg 11/19/14
modulesdir=$BES_MODULE_DIR
AC_SUBST(modulesdir)

AC_CONFIG_FILES([Makefile])

AC_CONFIG_FILES([tests/Makefile tests/package.m4 tests/atlocal tests/bes.conf])
AC_CONFIG_FILES([tests/generate_data_baseline.sh], [chmod +x tests/generate_data_baseline.sh])

AC_CONFIG_TESTDIR(tests)

AC_OUTPUT
