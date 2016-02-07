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

#include <kinesixd_daemon.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include <libinput.h>
#include <libudev.h>
#include <pthread.h>

static const char   DEVICES_PATH[] = "/dev/input/";
static const int    GESTURE_DELTA = 10;

struct _EventPollerThread
{
    pthread_t thread_id;
    pthread_attr_t attr;
    int stop_issued;
    pthread_mutex_t stop_mutex;
};

struct _LibInput
{
    struct libinput_interface interface;
    struct libinput *instance;
    struct libinput_device *active_device;

    /* The absolute maximum value for swipe velocity */
    /* These help determine swipe direction */
    double swipe_x_max;
    double swipe_y_max;
};

struct _KinesixDaemon
{
    KinesixdDevice active_device;
    KinesixdDevice *valid_device_list;
    struct KinesixDaemonCallbacks callbacks;
    void *user_data;

    int gesture_type;
    struct _LibInput libinput;
    struct _EventPollerThread event_poller_thread;
};

typedef enum
{
    GestureStarted,
    GestureOngoing,
    GestureFinished,
    GestureStateUnknown
} GestureEventState;

typedef enum
{
    GestureSwipe,
    GesturePinch,
    GestureUnknown
} GestureType;

static void *kinesixd_daemon_priv_poll_events(void *kinesixd_daemon);
static int libinput_open_restricted(const char *path, int flags, void *user_data);
static void libinput_close_restricted(int fd, void *user_data);

KinesixDaemon kinesixd_daemon_new(const struct KinesixDaemonCallbacks callbacks, void *user_data)
{
    KinesixDaemon self = (KinesixDaemon)malloc(sizeof(struct _KinesixDaemon));
    self->active_device = 0;
    self->valid_device_list = 0;
    self->callbacks = callbacks;
    self->user_data = user_data;

    self->gesture_type = UNKNOWN_GESTURE;

    self->libinput.interface.open_restricted = &libinput_open_restricted;
    self->libinput.interface.close_restricted = &libinput_close_restricted;
    self->libinput.instance = libinput_path_create_context(&self->libinput.interface, 0);
    self->libinput.swipe_x_max = 0;
    self->libinput.swipe_y_max = 0;

    pthread_attr_init(&self->event_poller_thread.attr);
    pthread_attr_setdetachstate(&self->event_poller_thread.attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&self->event_poller_thread.stop_mutex, 0);
    self->event_poller_thread.stop_issued = 0;

    /* TODO:                                                                                      */
    /* It might be usefull to set up inotify for /dev/input in order to detect new devices        */
    /* For now we stick to a static list initialized at the same time as the GestureDeamon itself */
    self->valid_device_list = kinesixd_daemon_get_valid_device_list(self);

    return self;
}

void kinesixd_daemon_free(KinesixDaemon self)
{
    kinesixd_daemon_stop_polling(self);
    pthread_attr_destroy(&self->event_poller_thread.attr);

    if (self->libinput.active_device)
        libinput_path_remove_device(self->libinput.active_device);
    libinput_unref(self->libinput.instance);
    kinesixd_device_list_free(self->valid_device_list);

    free(self);
}

void kinesixd_daemon_set_active_device(KinesixDaemon self, KinesixdDevice device)
{
    if (!kinesixd_device_equals(self->active_device, device))
    {
        if (kinesixd_device_list_contains(self->valid_device_list, device))
        {
            if (self->libinput.active_device)
                libinput_path_remove_device(self->libinput.active_device);

            self->active_device = device;
            self->libinput.active_device = libinput_path_add_device(self->libinput.instance, kinesixd_device_get_path(device));
        }
        else
        {
            LOG_ERROR("Device %s is not a valid device", kinesixd_device_get_path(device));
        }
    }
    else
    {
        LOG_WARN("Device %s is already active", kinesixd_device_get_path(device));
    }
}

void kinesixd_daemon_start_polling(KinesixDaemon self)
{
    self->event_poller_thread.stop_issued = 0;
    pthread_create(&self->event_poller_thread.thread_id,
                   &self->event_poller_thread.attr,
                   &kinesixd_daemon_priv_poll_events,
                   (void *)self
    );
}

void kinesixd_daemon_stop_polling(KinesixDaemon self)
{
    pthread_mutex_lock(&self->event_poller_thread.stop_mutex);
    self->event_poller_thread.stop_issued = 1;
    pthread_mutex_unlock(&self->event_poller_thread.stop_mutex);
    pthread_join(self->event_poller_thread.thread_id, 0);
}

static void kinesixd_daemon_priv_sanitize_device_name(const char *device_name, char *buffer, size_t buffer_size)
{
    int stop = 0;
    size_t device_name_it = 0;
    size_t buffer_it = 0;
    size_t undeline_counter = 0;

    for (; !stop; ++device_name_it, ++buffer_it)
    {
        if ((device_name[device_name_it] == '\0') || (buffer_it == buffer_size - 1))
            stop = 1;

        if (device_name[device_name_it] == '_')
        {
            if (!undeline_counter)
            {
                ++undeline_counter;
                buffer[buffer_it] = ' ';
            }
            else
                --buffer_it;
        }
        else
        {
            if (undeline_counter)
                undeline_counter = 0;

            buffer[buffer_it] = device_name[device_name_it];
        }
    }

    buffer[buffer_it] = '\0';
}

static void kinesixd_daemon_priv_add_device(const KinesixDaemon self,
                                            const char *device_name,
                                            KinesixdDevice **device_list_out,
                                            int *current_index_out)
{
    KinesixdDevice new_device = 0;
    struct libinput_device *libinput_dev = 0;
    char device_path[strlen(DEVICES_PATH) + strlen(device_name) + 1];
    struct udev_device *udev_dev = 0;
    const char *udev_name = 0;
    size_t buffer_size = 100;
    char udev_dev_sanatized_name[100];

    sprintf(device_path,"%s%s", DEVICES_PATH, device_name);
    libinput_dev = libinput_path_add_device(self->libinput.instance, device_path);
    if (libinput_dev)
    {
        if (libinput_device_has_capability(libinput_dev, LIBINPUT_DEVICE_CAP_GESTURE))
        {
            if ((udev_dev = libinput_device_get_udev_device(libinput_dev)))
                udev_name = udev_device_get_property_value(udev_dev, "ID_MODEL");

            if (!udev_name)
            {
                udev_name = libinput_device_get_name(libinput_dev);
            }
            else
            {
                kinesixd_daemon_priv_sanitize_device_name(udev_name, udev_dev_sanatized_name, buffer_size);
                udev_name = udev_dev_sanatized_name;
            }

            new_device = kinesixd_device_new(device_path,
                                             udev_name,
                                             libinput_device_get_id_product(libinput_dev),
                                             libinput_device_get_id_vendor(libinput_dev));
            if (new_device)
                *device_list_out[(*current_index_out)++] = new_device;
        }
        libinput_path_remove_device(libinput_dev);
    }
}

static KinesixdDevice *kinesixd_daemon_priv_device_list_duplicate(const KinesixdDevice *device_list, int size)
{
    KinesixdDevice *result;
    int i;

    result = (KinesixdDevice *)malloc((size + 1) * sizeof(KinesixdDevice));

    for (i = 0; i < size; ++i)
    {
        result[i] = device_list[i];
    }

    result[size] = 0;

    return result;
}

KinesixdDevice *kinesixd_daemon_get_valid_device_list(const KinesixDaemon self)
{
    KinesixdDevice *device_list_heap = self->valid_device_list;

    if (!self->valid_device_list)
    {
        int device_count = 0;
        KinesixdDevice device_list[255];
        KinesixdDevice *device_list_ptr = &device_list[0];
        struct dirent *file = 0;
        DIR *dir = 0;

        if ((dir = opendir(DEVICES_PATH)) != 0)
        {
            for (;;)
            {
                file = readdir(dir);

                /* Check to see if there are files left to check */
                if (file == 0)
                    break;

                /* Check to see if file is a characted device */
                if (file->d_type == DT_CHR)
                    kinesixd_daemon_priv_add_device(self, file->d_name, &device_list_ptr, &device_count);
            }
        }
        free(dir);

        device_list_heap = kinesixd_daemon_priv_device_list_duplicate(device_list, device_count);
    }

    return device_list_heap;
}

static int libinput_open_restricted(const char *path, int flags, void *user_data)
{
    int fd = -1;

    UNUSED(user_data)

    if ((fd = open(path, flags)) == -1)
    {
        LOG_FATAL("Failed to open file descriptor at %s. %s", path, strerror(errno));
    }

    return fd;
}

static void libinput_close_restricted(int fd, void *user_data)
{
    UNUSED(user_data)

    close(fd);
}

static int kinesixd_daemon_priv_handle_swipe_update(KinesixDaemon self,
                                                    struct libinput_event_gesture *gesture_event)
{
    double x_max = self->libinput.swipe_x_max;
    double y_max = self->libinput.swipe_y_max;
    double x_current = 0;
    double y_current = 0;
    int swipe_direction = UNKNOWN_GESTURE;

    if (!gesture_event)
        return swipe_direction;

    x_current = libinput_event_gesture_get_dx_unaccelerated(gesture_event);
    y_current = libinput_event_gesture_get_dy_unaccelerated(gesture_event);

    x_max = fabs(x_max) < fabs(x_current) ? x_current : x_max;
    y_max = fabs(y_max) < fabs(y_current) ? y_current : y_max;

    if (fabs(y_max) > fabs(x_max))
    {
        if (y_max < -GESTURE_DELTA)
        {
            swipe_direction = SWIPE_UP;
        }
        else if (y_max > GESTURE_DELTA)
        {
            swipe_direction = SWIPE_DOWN;
        }
    }
    else if (fabs(x_max) > fabs(y_max))
    {
        if (x_max < -GESTURE_DELTA)
        {
           swipe_direction = SWIPE_LEFT;
        }
        else if (x_max > GESTURE_DELTA)
        {
            swipe_direction = SWIPE_RIGHT;
        }
    }

    self->libinput.swipe_x_max = x_max;
    self->libinput.swipe_y_max = y_max;

    return swipe_direction;
}

static int kinesixd_daemon_priv_handle_pinch_update(KinesixDaemon self,
                                                    struct libinput_event_gesture *gesture_event)
{
    UNUSED(self)

    double scale = 1;
    int pinch_type = UNKNOWN_GESTURE;

    if (!gesture_event)
        return pinch_type;

    scale = libinput_event_gesture_get_scale(gesture_event);

    if (scale > 1)
        pinch_type = PINCH_OUT;
    else if (scale < 1)
        pinch_type = PINCH_IN;

    return pinch_type;
}

static GestureEventState kinesixd_daemon_priv_handle_swipe(KinesixDaemon self,
                                                 struct libinput_event *event,
                                                 int *swipe_finger_count_out)
{
    struct libinput_event_gesture *gesture_event = 0;
    enum libinput_event_type gesture_event_type;
    int swipe_finger_count = 0;
    GestureEventState state = GestureStateUnknown;

    if (!event)
        return state;

    gesture_event_type = libinput_event_get_type(event);

    if (gesture_event_type == LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        swipe_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        state = GestureStarted;
    }
    else if (gesture_event_type == LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        swipe_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        self->gesture_type = kinesixd_daemon_priv_handle_swipe_update(self, gesture_event);
        state = GestureOngoing;
    }
    else if (gesture_event_type == LIBINPUT_EVENT_GESTURE_SWIPE_END)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        swipe_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        state = GestureFinished;
        self->libinput.swipe_x_max = 0;
        self->libinput.swipe_y_max = 0;
    }

    *swipe_finger_count_out = swipe_finger_count;

    return state;
}

static GestureEventState kinesixd_daemon_priv_handle_pinch(KinesixDaemon self,
                                                   struct libinput_event *event,
                                                   int *pinch_finger_count_out)
{
    struct libinput_event_gesture *gesture_event = 0;
    enum libinput_event_type gesture_event_type;
    int pinch_finger_count = 0;
    GestureEventState state = GestureStateUnknown;

    if (!event)
        return state;

    gesture_event_type = libinput_event_get_type(event);

    if (gesture_event_type == LIBINPUT_EVENT_GESTURE_PINCH_BEGIN)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        pinch_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        state = GestureStarted;
    }
    else if (gesture_event_type == LIBINPUT_EVENT_GESTURE_PINCH_UPDATE)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        pinch_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        self->gesture_type = kinesixd_daemon_priv_handle_pinch_update(self, gesture_event);
        state = GestureOngoing;
    }
    else if (gesture_event_type == LIBINPUT_EVENT_GESTURE_PINCH_END)
    {
        gesture_event = libinput_event_get_gesture_event(event);
        pinch_finger_count = libinput_event_gesture_get_finger_count(gesture_event);
        state = GestureFinished;
        self->libinput.swipe_x_max = 0;
        self->libinput.swipe_y_max = 0;
    }

    *pinch_finger_count_out = pinch_finger_count;

    return state;
}

static void kinesixd_daemon_priv_handle_gesture(KinesixDaemon self,
                                                   struct libinput_event *event)
{
    int finger_count = 0;
    GestureType gesture_type = GestureUnknown;
    GestureEventState gesture_state = GestureStateUnknown;

    gesture_state = kinesixd_daemon_priv_handle_swipe(self,
                                                      event,
                                                      &finger_count);
    if (gesture_state != GestureStateUnknown)
    {
        gesture_type = GestureSwipe;
    }
    else
    {
        gesture_state = kinesixd_daemon_priv_handle_pinch(self,
                                                          event,
                                                          &finger_count);
        if (gesture_state != GestureStateUnknown)
                gesture_type = GesturePinch;
    }

    if ((gesture_state == GestureFinished) &&
        (libinput_event_gesture_get_cancelled(
             libinput_event_get_gesture_event(event)) == 0))
    {
        if ((gesture_type == GestureSwipe) && (self->callbacks.swiped_cb != 0))
            self->callbacks.swiped_cb(self->gesture_type, finger_count, self->user_data);
        if ((gesture_type == GesturePinch) && (self->callbacks.pinch_cb!= 0))
            self->callbacks.pinch_cb(self->gesture_type, finger_count, self->user_data);
    }

    libinput_event_destroy(event);
}

static void *kinesixd_daemon_priv_poll_events(void *kinesixd_daemon)
{
    KinesixDaemon self = (KinesixDaemon)kinesixd_daemon;
    int stop_issued = 0;

    struct pollfd poller = {
        .fd = libinput_get_fd(self->libinput.instance),
        .events = POLLIN,
        .revents = 0
    };

    for (;;)
    {
        pthread_mutex_lock(&self->event_poller_thread.stop_mutex);
        stop_issued = self->event_poller_thread.stop_issued;
        pthread_mutex_unlock(&self->event_poller_thread.stop_mutex);

        if (stop_issued)
            break;

        /* Wait for an event to be ready by polling the internal libinput fd */
        poll(&poller, 1, 500);

        if (poller.revents == POLLIN)
        {
            /* Notify libinput that an event is ready and to add it (hopefully) to the event queue */
            libinput_dispatch(self->libinput.instance);

            /* Get the actual event from the queue and send it for processing*/
            kinesixd_daemon_priv_handle_gesture(self,
                                 libinput_get_event(self->libinput.instance));
        }
    }

    pthread_exit(0);
}
