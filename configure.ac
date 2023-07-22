AC_INIT([bobbin], [v0.1.1-dev], [micah@addictivecode.org])
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AM_PATH_PYTHON(3,,[:])
AC_PATH_PROG([CA65], [ca65], [:])
AC_PATH_PROG([LD65], [ld65], [:])
AC_SUBST(CA65)
AC_SUBST(LD65)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
    test/tests6502/Makefile
    test/pexpect/Makefile
])
AC_OUTPUT
