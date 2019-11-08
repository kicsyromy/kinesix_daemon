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

#ifndef DEVICEMARSHALER_H
#define DEVICEMARSHALER_H

#include <dbus/dbus.h>

#include "kinesixd_device.h"
#include "kinesixd_global.h"

int kinesixd_device_marshaler_append_device(const KinesixdDevice device, DBusMessageIter *dbus_iter);
KinesixdDevice kinesixd_device_marshaler_device_from_dbus_argument(DBusMessageIter *dbus_iter);
int kinesixd_device_marshaler_append_device_list(const KinesixdDevice *const device_list, DBusMessageIter *dbus_iter);

#endif // DEVICEMARSHALER_H
