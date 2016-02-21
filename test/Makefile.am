AUTOMAKE_OPTIONS = foreign

CFLAGS   = -g -O0 -pipe -Wall -W -fPIC -I. -I../src @LIBINPUT_CFLAGS@ @LIBUDEV_CFLAGS@ @LIBDBUS_CFLAGS@
LDFLAGS  = -pthread $(LIBINPUT_LIBS) $(LIBUDEV_LIBS) $(LIBDBUS_LIBS)

bin_PROGRAMS = test_kinesixd
test_kinesixd_SOURCES = main.c \
                ../src/kinesixd_daemon.c \
                ../src/kinesixd_device.c \
                ../src/kinesixd_device_marshaler.c \
                ../src/kinesixd_dbus_adaptor.c
