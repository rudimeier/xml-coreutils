AC_PREREQ(2.59)
AC_INIT([xml-coreutils],[0.8],[laird@lbreyer.com])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
AC_GNU_SOURCE
AM_INIT_AUTOMAKE([xml-coreutils],[0.8],[laird@lbreyer.com])
AM_CONFIG_HEADER(src/config.h)

## Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LEX
AC_PROG_YACC

## Checks for libraries.
AC_CHECK_LIB([m],[log])
AC_CHECK_LIB([expat],[XML_ParseBuffer],,[AC_MSG_ERROR([Missing libexpat])])
AC_CHECK_LIB([slang],[SLsmg_init_smg],,[AC_MSG_ERROR([Missing libslang2])])

## Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([features.h langinfo.h unistd.h sys/mman.h mman.h netinet/in.h])
AC_CHECK_HEADERS([wchar.h wctype.h],,
[
	AC_MSG_WARN([No wide character headers, disabling full internationalization.])
])
AC_CHECK_HEADERS([expat.h slang.h],,[AC_MSG_ERROR([Missing expat.h or slang.h])])

## Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_OFF_T

## Checks for library functions.
AC_FUNC_VPRINTF
AC_FUNC_SETMODE_DOS
AC_FUNC_MBRTOWC
AC_CHECK_FUNCS([sigaction])

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile src/tests/Makefile man/Makefile])
AC_OUTPUT
