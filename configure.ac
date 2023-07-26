AC_INIT([bobbin], [v0.2.1-dev], [micah@addictivecode.org])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AM_PATH_PYTHON(3,,[:])
AC_PATH_PROG([CA65], [ca65], [:])
AC_PATH_PROG([LD65], [ld65], [:])
AC_SUBST(CA65)
AC_SUBST(LD65)

AC_ARG_WITH([editline],
  [AS_HELP_STRING([--with-editline],
    [support fancy command line editing @<:@default=check@:>@])],
  [],
  [with_editline=check])

LIBEDITLINE=
AS_IF([test "x$with_editline" != xno],
  [AC_CHECK_LIB([edit], [el_gets],
    [AC_SUBST([LIBEDITLINE], ["-ledit -ltermcap"])
     AC_DEFINE([HAVE_LIBEDITLINE], [1],
               [Define if you have libeditline])
    ],
    [if test "x$with_editline" != xcheck; then
       AC_MSG_FAILURE(
         [--with-editline was given, but test for editline failed])
     fi
    ], -ltermcap)])

AC_CONFIG_HEADERS([src/ac-config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
    test/tests6502/Makefile
    test/pexpect/Makefile
])
AC_OUTPUT
