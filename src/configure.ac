AC_INIT([kinesixd], [0.1])
AM_INIT_AUTOMAKE([kinesixd], [0.1])
AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC

AC_CHECK_HEADER(stdlib.h)
AC_CHECK_HEADER(string.h)
AC_CHECK_HEADER(math.h)
AC_CHECK_HEADER(stdio.h)
AC_CHECK_HEADER(errno.h)
AC_CHECK_HEADER(unistd.h)
AC_CHECK_HEADER(dirent.h)
AC_CHECK_HEADER(sys/types.h)
AC_CHECK_HEADER(sys/stat.h)
AC_CHECK_HEADER(fcntl.h)
AC_CHECK_HEADER(poll.h)
AC_CHECK_HEADER(pthread.h)

AC_FUNC_MALLOC
AC_CHECK_FUNCS([strdup strerror strrchr])

PKG_CHECK_MODULES([LIBINPUT], [libinput])
PKG_CHECK_MODULES([LIBUDEV], [libudev])
PKG_CHECK_MODULES([LIBDBUS], [dbus-1])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

