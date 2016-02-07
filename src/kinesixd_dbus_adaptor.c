/*
 * Copyright © 2015 Romeo Calota
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Romeo Calota
 */

#include "kinesixd_dbus_adaptor.h"

#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>

#include "kinesixd_daemon.h"
#include "kinesixd_device_marshaler.h"
#include "kinesixd_device_p.h"

#ifdef DEBUG_BUILD
static const char *swipe_directions[] = { "Up", "Down", "Left", "Right" };
static const char *pinch_types[]      = { "In", "Out" };
#endif

static const char GESTURE_DAEMON_DBUS_NAME[]        = "org.kicsyromy.kinesixd";
static const char GESTURE_DAEMON_OBJECT_PATH[]      = "/org/kicsyromy/kinesixd";
static const char GESTURE_DAEMON_INTERFACE_NAME[]   = "org.kicsyromy.kinesixd";

static const char GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_ROOT[] = ""
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node><node name=\"org\"/></node>";

static const char GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_ORG[] = ""
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node><node name=\"kicsyromy\"/></node>";

static const char GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_KICSYROMY[] = ""
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node><node name=\"kinesixd\"/></node>";

static const char GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_KINESIXD[] = ""
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
"\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node>"
    "<interface name=\"org.freedesktop.DBus.Introspectable\">"
        "<method name=\"Introspect\">"
            "<arg type=\"s\" name=\"xml_data\" direction=\"out\"/>"
        "</method>"
    "</interface>"
    "<interface name=\"org.kicsyromy.kinesixd\">"
        "<signal name=\"Swiped\">"
            "<arg name=\"direction\" type=\"i\" direction=\"out\"/>"
            "<arg name=\"finger_count\" type=\"i\" direction=\"out\"/>"
        "</signal>"
        "<signal name=\"Pinch\">"
            "<arg name=\"pinch_type\" type=\"i\" direction=\"out\"/>"
            "<arg name=\"finger_count\" type=\"i\" direction=\"out\"/>"
        "</signal>"
        "<method name=\"GetValidDeviceList\">"
            "<arg type=\"a(issuu)\" direction=\"out\"/>"
        "</method>"
        "<method name=\"SetActiveDevice\">"
            "<annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>"
            "<arg name=\"device\" type=\"(issuu)\" direction=\"in\"/>"
        "</method>"
    "</interface>"
"</node>";

struct _MessageListenerThread
{
    pthread_t thread_id;
    pthread_attr_t attr;
    int stop_issued;
    pthread_mutex_t stop_mutex;
    pthread_mutex_t signal_mutex;
};

struct _DBus
{
    DBusError error;
    DBusConnection *connection;
    struct _MessageListenerThread message_listener;
};

struct _KinesixdDBusAdaptor
{
    KinesixDaemon kinesixd_daemon;
    struct _DBus d_bus;
};

static void kinesixd_dbus_adaptor_priv_swiped(int direction, int finger_count, void *kinesixd_dbus_adaptor);
static void kinesixd_dbus_adaptor_priv_pinch(int pinch_type, int finger_count, void *kinesixd_dbus_adaptor);
static void kinesixd_dbus_adaptor_get_valid_device_list(KinesixdDBusAdaptor kinesixd_dbus_adaptor,
                                                              DBusMessage *message);
static void kinesixd_dbus_adaptor_set_active_device(KinesixdDBusAdaptor kinesixd_dbus_adaptor,
                                                          DBusMessage *message);
static void kinesixd_dbus_adaptor_handle_introspection(KinesixdDBusAdaptor kinesixd_dbus_adaptor,
                                                            DBusMessage *message);
static void kinesixd_dbus_adaptor_handle_unkown_message(KinesixdDBusAdaptor kinesixd_dbus_adaptor,
                                                              DBusMessage *message);
static void *kinesixd_dbus_adaptor_priv_listen_for_messages(void *kinesixd_dbus_adaptor);

KinesixdDBusAdaptor kinesixd_dbus_adaptor_new(DBusBusType type)
{
    KinesixdDBusAdaptor self = (KinesixdDBusAdaptor)malloc(sizeof(struct _KinesixdDBusAdaptor));
    struct KinesixDaemonCallbacks callbacks =
    {
        .swiped_cb = &kinesixd_dbus_adaptor_priv_swiped,
        .pinch_cb  = &kinesixd_dbus_adaptor_priv_pinch
    };

    self->kinesixd_daemon = kinesixd_daemon_new(callbacks, self);

    dbus_error_init(&self->d_bus.error);
    self->d_bus.connection = dbus_bus_get(type, &self->d_bus.error);
    if (dbus_error_is_set(&self->d_bus.error))
    {
        LOG_FATAL("Error connecting to DBus. %s.", self->d_bus.error.message);
    }
    else if (self->d_bus.connection)
    {
        dbus_bus_request_name(self->d_bus.connection,
                              GESTURE_DAEMON_DBUS_NAME,
                              DBUS_NAME_FLAG_REPLACE_EXISTING,
                              &self->d_bus.error);
        if (dbus_error_is_set(&self->d_bus.error))
        {
            dbus_error_free(&self->d_bus.error);
            dbus_connection_unref(self->d_bus.connection);
            LOG_FATAL("Error acquiring DBus name. %s", self->d_bus.error.message);
        }

        pthread_attr_init(&self->d_bus.message_listener.attr);
        pthread_attr_setdetachstate(&self->d_bus.message_listener.attr, PTHREAD_CREATE_JOINABLE);
        pthread_mutex_init(&self->d_bus.message_listener.stop_mutex, 0);
        pthread_mutex_init(&self->d_bus.message_listener.signal_mutex, 0);
        self->d_bus.message_listener.stop_issued = 0;
    }

    return self;
}

void kinesixd_dbus_adaptor_free(KinesixdDBusAdaptor self)
{
    kinesixd_dbus_adaptor_stop_listenting(self);
    pthread_attr_destroy(&self->d_bus.message_listener.attr);

    kinesixd_daemon_free(self->kinesixd_daemon);

    dbus_error_free(&self->d_bus.error);
    if (self->d_bus.connection)
        dbus_connection_unref(self->d_bus.connection);

    free(self);
}

void kinesixd_dbus_adaptor_start_listenting(KinesixdDBusAdaptor self)
{
    kinesixd_daemon_start_polling(self->kinesixd_daemon);
    self->d_bus.message_listener.stop_issued = 0;
    pthread_create(&self->d_bus.message_listener.thread_id,
                   &self->d_bus.message_listener.attr,
                   &kinesixd_dbus_adaptor_priv_listen_for_messages,
                   (void *)self
    );
}

void kinesixd_dbus_adaptor_stop_listenting(KinesixdDBusAdaptor self)
{
    pthread_mutex_lock(&self->d_bus.message_listener.stop_mutex);
    self->d_bus.message_listener.stop_issued = 1;
    pthread_mutex_unlock(&self->d_bus.message_listener.stop_mutex);
    kinesixd_daemon_stop_polling(self->kinesixd_daemon);
    pthread_join(self->d_bus.message_listener.thread_id, 0);
}

static void kinesixd_dbus_adaptor_priv_swiped(int direction, int finger_count, void *kinesixd_dbus_adaptor)
{
    KinesixdDBusAdaptor self = (KinesixdDBusAdaptor)kinesixd_dbus_adaptor;
    dbus_uint32_t reply_id = 0;
    DBusMessage *message = 0;

    LOG_DEBUG("Swiped with %d fingers in direction %s", finger_count, swipe_directions[direction]);

    message = dbus_message_new_signal(GESTURE_DAEMON_OBJECT_PATH,
                                      GESTURE_DAEMON_INTERFACE_NAME,
                                      "Swiped");
    if (!message)
    {
        LOG_ERROR("Could not create DBus message. Unable to send signal %s.Swiped(%d, %d)",
                  GESTURE_DAEMON_INTERFACE_NAME,
                  direction,
                  finger_count);

        return;
    }

    if (!dbus_message_append_args(message,
                                  DBUS_TYPE_INT32, &direction,
                                  DBUS_TYPE_INT32, &finger_count,
                                  DBUS_TYPE_INVALID))
    {
        LOG_ERROR("Could not append agruments to signal. Probably out of memory.");
        return;
    }

    pthread_mutex_lock(&self->d_bus.message_listener.signal_mutex);
    if (!dbus_connection_send(self->d_bus.connection, message, &reply_id))
    {
        LOG_ERROR("Failed to send DBus signal %s.Swiped(%d, %d). Probably out of memory.",
                  GESTURE_DAEMON_INTERFACE_NAME,
                  direction,
                  finger_count);
    }
    else
        dbus_connection_flush(self->d_bus.connection);
    pthread_mutex_unlock(&self->d_bus.message_listener.signal_mutex);

    dbus_message_unref(message);
}

static void kinesixd_dbus_adaptor_priv_pinch(int pinch_type, int finger_count, void *kinesixd_dbus_adaptor)
{
    KinesixdDBusAdaptor self = (KinesixdDBusAdaptor)kinesixd_dbus_adaptor;
    dbus_uint32_t reply_id = 0;
    DBusMessage *message = 0;

    LOG_DEBUG("Pinch %s with %d fingers", pinch_types[pinch_type], finger_count);

    message = dbus_message_new_signal(GESTURE_DAEMON_OBJECT_PATH,
                                      GESTURE_DAEMON_INTERFACE_NAME,
                                      "Pinch");
    if (!message)
    {
        LOG_ERROR("Could not create DBus message. Unable to send signal %s.Pinch(%d, %d)",
                  GESTURE_DAEMON_INTERFACE_NAME,
                  pinch_type,
                  finger_count);

        return;
    }

    if (!dbus_message_append_args(message,
                                  DBUS_TYPE_INT32, &pinch_type,
                                  DBUS_TYPE_INT32, &finger_count,
                                  DBUS_TYPE_INVALID))
    {
        LOG_ERROR("Could not append agruments to signal. Probably out of memory.");
        return;
    }

    pthread_mutex_lock(&self->d_bus.message_listener.signal_mutex);
    if (!dbus_connection_send(self->d_bus.connection, message, &reply_id))
    {
        LOG_ERROR("Failed to send DBus signal %s.Pinch(%d, %d). Probably out of memory.",
                  GESTURE_DAEMON_INTERFACE_NAME,
                  pinch_type,
                  finger_count);
    }
    else
        dbus_connection_flush(self->d_bus.connection);
    pthread_mutex_unlock(&self->d_bus.message_listener.signal_mutex);

    dbus_message_unref(message);
}

static void kinesixd_dbus_adaptor_get_valid_device_list(KinesixdDBusAdaptor self,
                                                              DBusMessage *message)
{
    DBusMessage* reply = 0;
    DBusMessageIter reply_args;

    LOG_DEBUG("Called %s.%s on %s",
              dbus_message_get_interface(message),
              dbus_message_get_member(message),
              dbus_message_get_path(message));

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &reply_args);

    KinesixdDevice *device_list = kinesixd_daemon_get_valid_device_list(self->kinesixd_daemon);
    kinesixd_device_marshaler_append_device_list(device_list, &reply_args);

    if (!dbus_connection_send(self->d_bus.connection, reply, 0))
    {
        LOG_ERROR("Failed send reply for %s.%s called by %s on %s",
                  dbus_message_get_interface(message),
                  dbus_message_get_member(message),
                  dbus_message_get_sender(message),
                  dbus_message_get_path(message));
    }
    else
    {
        dbus_connection_flush(self->d_bus.connection);
    }

    dbus_message_unref(reply);
}

static void kinesixd_dbus_adaptor_set_active_device(KinesixdDBusAdaptor self,
                                                          DBusMessage *message)
{
    DBusMessage* reply = 0;
    DBusMessageIter message_arg;
    KinesixdDevice device = 0;

    LOG_DEBUG("Called %s.%s on %s",
              dbus_message_get_interface(message),
              dbus_message_get_member(message),
              dbus_message_get_path(message));


    /* Send back an empty reply */
    reply = dbus_message_new_method_return(message);
    if (!dbus_connection_send(self->d_bus.connection, reply, 0))
        LOG_ERROR("Failed to send reply");
    else
        dbus_connection_flush(self->d_bus.connection);

    if (!dbus_message_iter_init(message, &message_arg))
    {
        LOG_ERROR("No arguments available");
    }
    else
    {
        device = kinesixd_device_marshaler_device_from_dbus_argument(&message_arg);
        if (device)
            kinesixd_daemon_set_active_device(self->kinesixd_daemon, device);
    }

    dbus_message_unref(reply);
}

static void kinesixd_dbus_adaptor_handle_introspection(KinesixdDBusAdaptor self,
                                                             DBusMessage *message)
{
    DBusMessage* reply = 0;
    char *introspection_data = 0;
    const char *object_path = dbus_message_get_path(message);

    if (strcmp(object_path, "/") == 0)
        introspection_data = strdup(GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_ROOT);
    else if (strcmp(object_path, "/org") == 0)
        introspection_data = strdup(GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_ORG);
    else if (strcmp(object_path, "/org/kicsyromy") == 0)
        introspection_data = strdup(GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_KICSYROMY);
    else if (strcmp(object_path, GESTURE_DAEMON_OBJECT_PATH) == 0)
        introspection_data = strdup(GESTURE_DAEMON_DBUS_INTROSPECTION_DATA_KINESIXD);

    reply = dbus_message_new_method_return(message);
    if (!dbus_message_append_args(reply,
                             DBUS_TYPE_STRING,
                             &introspection_data,
                             DBUS_TYPE_INVALID))
    {
        LOG_ERROR("Failed create reply for %s.%s called by %s on %s",
                  dbus_message_get_interface(message),
                  dbus_message_get_member(message),
                  dbus_message_get_sender(message),
                  dbus_message_get_path(message));
    }
    else if (!dbus_connection_send(self->d_bus.connection, reply, 0))
    {
        LOG_ERROR("Failed send reply for %s.%s called by %s on %s",
                  dbus_message_get_interface(message),
                  dbus_message_get_member(message),
                  dbus_message_get_sender(message),
                  dbus_message_get_path(message));
    }
    else
    {
        dbus_connection_flush(self->d_bus.connection);
    }
    dbus_message_unref(reply);
    free(introspection_data);
}

static void kinesixd_dbus_adaptor_handle_unkown_message(KinesixdDBusAdaptor self,
                                                              DBusMessage *message)
{
    DBusMessage* reply = 0;

    LOG_WARN("Unhadled method called");

    reply = dbus_message_new_method_return(message);
    if (!dbus_connection_send(self->d_bus.connection, reply, 0))
    {
        LOG_ERROR("Failed to send reply for %s.%s called by %s on %s",
                  dbus_message_get_interface(message),
                  dbus_message_get_member(message),
                  dbus_message_get_sender(message),
                  dbus_message_get_path(message));
    }
    else
    {
        dbus_connection_flush(self->d_bus.connection);
    }
    dbus_message_unref(reply);
}

static void *kinesixd_dbus_adaptor_priv_listen_for_messages(void *kinesixd_dbus_adaptor)
{
    KinesixdDBusAdaptor self = (KinesixdDBusAdaptor)kinesixd_dbus_adaptor;
    int stop_issued = 0;
    DBusMessage *message = 0;

    for (;;)
    {
        pthread_mutex_lock(&self->d_bus.message_listener.stop_mutex);
        stop_issued = self->d_bus.message_listener.stop_issued;
        pthread_mutex_unlock(&self->d_bus.message_listener.stop_mutex);

        if (stop_issued)
            break;

        /* Avoid blocking in critical section */
        usleep(500);
        pthread_mutex_lock(&self->d_bus.message_listener.signal_mutex);
        dbus_connection_read_write(self->d_bus.connection, 0);
        message = dbus_connection_pop_message(self->d_bus.connection);
        pthread_mutex_unlock(&self->d_bus.message_listener.signal_mutex);

        if (!message)
            continue;

        LOG_DEBUG("Method %s.%s called by %s on %s",
                 dbus_message_get_interface(message),
                 dbus_message_get_member(message),
                 dbus_message_get_sender(message),
                 dbus_message_get_path(message));

        if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
            kinesixd_dbus_adaptor_handle_introspection(self, message);
        else if (dbus_message_is_method_call(message, GESTURE_DAEMON_INTERFACE_NAME, "GetValidDeviceList"))
            kinesixd_dbus_adaptor_get_valid_device_list(self, message);
        else if (dbus_message_is_method_call(message, GESTURE_DAEMON_INTERFACE_NAME, "SetActiveDevice"))
            kinesixd_dbus_adaptor_set_active_device(self, message);
        else
            kinesixd_dbus_adaptor_handle_unkown_message(self, message);

        dbus_message_unref(message);
        message = 0;
    }

    pthread_exit(0);
}
