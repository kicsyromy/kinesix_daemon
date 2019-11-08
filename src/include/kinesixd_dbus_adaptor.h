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

#ifndef GESTUREDAEMONDBUSADAPTOR_H
#define GESTUREDAEMONDBUSADAPTOR_H

#include <dbus/dbus.h>

#include "kinesixd_global.h"

typedef struct _KinesixdDBusAdaptor * KinesixdDBusAdaptor;

KinesixdDBusAdaptor kinesixd_dbus_adaptor_new(DBusBusType type);
void kinesixd_dbus_adaptor_free(KinesixdDBusAdaptor dbus_adaptor);
void kinesixd_dbus_adaptor_start_listenting(KinesixdDBusAdaptor dbus_adaptor);
void kinesixd_dbus_adaptor_stop_listenting(KinesixdDBusAdaptor dbus_adaptor);

#endif // GESTUREDAEMONDBUSADAPTOR_H
