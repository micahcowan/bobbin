AC_INIT([bobbin], [0.4-dev], [micah@addictivecode.org],,
        [https://github.com/micahcowan/bobbin/])
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE([foreign dist-zip])
AC_PROG_CC
AC_PATH_PROG([CA65], [ca65], [:])
AC_PATH_PROG([LD65], [ld65], [:])
AC_SUBST(CA65)
AC_SUBST(LD65)

dnl Needed to get SIGWINCH on MacOS. Why on earth it would hide
dnl non-interfering features just because _POSIX_C_SOURCE is defined...
AC_USE_SYSTEM_EXTENSIONS

AC_ARG_WITH([curses],
    [AS_HELP_STRING([--without-curses],
        [Build without the curses/ncurses library, disabling the default "tty" interface.])],
    [],
    [with_curses=yes])

LIBCURSES=
BOBBIN_MAYBE_TTY=
AS_IF([test "x$with_curses" != "xno"],
    [AC_CHECK_LIB([curses], [initscr],
        [AC_SUBST([LIBCURSES], ["-lcurses"])
         AC_SUBST([BOBBIN_MAYBE_TTY], ['interfaces/tty.$(OBJEXT)'])
         AC_DEFINE([HAVE_LIBCURSES], [1],
                   [Define if you have libcurses])
         ],
        [AC_MSG_FAILURE([libcurses check failed. Please install the development package for [n]curses on your system, or use --without-curses to configure without it (not recommended); the default interface for bobbin will be disabled.])]
    )])

AC_CHECK_FUNC([inotify_add_watch],[AC_CHECK_HEADERS([sys/inotify.h])])

AM_PATH_PYTHON([3],,[:])
AS_IF([test "x$PYTHON" != "x" -a "x$PYTHON" != "x:"],
    [AC_MSG_CHECKING([for python pexpect module])
     AS_IF(["$PYTHON" -c 'import pexpect' >/dev/null 2>&1],
           [AC_MSG_RESULT([yes])],
           [AC_MSG_RESULT([no])
            AC_MSG_WARN([pexpect not found, disabling python tests])
            PYTHON=''])])

AM_CONDITIONAL([HAVE_CA65], [test "$CA65" != ":" -a "$LD65" != ":"])
AM_CONDITIONAL([HAVE_PEXPECT], [test "x${PYTHON}" != x: -a "x${PYTHON}" != x])

AC_CONFIG_HEADERS([src/ac-config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
    test/noninteract/Makefile
    test/tests6502/Makefile
    test/pexpect/Makefile
])
AC_OUTPUT

AS_IF([test "x$LIBCURSES" = "x"],
    [AC_MSG_WARN([The default interface, "tty", will not be built, because libcurses is not configured!])])
