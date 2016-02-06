AUTOMAKE_OPTIONS = foreign

CFLAGS   = -pipe -Wall -W -fPIC -I. @LIBINPUT_CFLAGS@ @LIBUDEV_CFLAGS@ @LIBDBUS_CFLAGS@
LDFLAGS  = -pthread $(LIBINPUT_LIBS) $(LIBUDEV_LIBS) $(LIBDBUS_LIBS)

bin_PROGRAMS = kinesixd
kinesixd_SOURCES = main.c \
                kinesixd_daemon.c \
                kinesixd_device.c \
                kinesixd_device_marshaler.c \
                kinesixd_dbus_adaptor.c
