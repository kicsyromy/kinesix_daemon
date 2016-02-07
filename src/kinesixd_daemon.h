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

#ifndef GESTURE_DAEMON_H
#define GESTURE_DAEMON_H

#include <kinesixd_device.h>

#define SWIPE_UP         0
#define SWIPE_DOWN       1
#define SWIPE_LEFT       2
#define SWIPE_RIGHT      3

#define PINCH_IN         0
#define PINCH_OUT        1

#define UNKNOWN_GESTURE -1

typedef struct _KinesixDaemon * KinesixDaemon;

typedef void (*SwipedCallback)(int direction, int finger_count, void *user_data);
typedef void (*PinchCallback)(int pinch_type, int finger_count, void *user_data);

struct KinesixDaemonCallbacks
{
    SwipedCallback swiped_cb;
    PinchCallback  pinch_cb;
};

KinesixDaemon kinesixd_daemon_new(const struct KinesixDaemonCallbacks callbacks, void *user_data);
void kinesixd_daemon_free(KinesixDaemon daemon);
KinesixdDevice *kinesixd_daemon_get_valid_device_list(const KinesixDaemon Daemon);
void kinesixd_daemon_set_active_device(KinesixDaemon daemon, KinesixdDevice device);
void kinesixd_daemon_start_polling(KinesixDaemon daemon);
void kinesixd_daemon_stop_polling(KinesixDaemon daemon);

#endif // GESTURE_DAEMON_H
