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

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#include <kinesixd_global.h>

typedef struct _KinesixdDevice * KinesixdDevice;

KinesixdDevice kinesixd_device_new(const char *path,
                  const char *name,
                  uint32_t product_id,
                  uint32_t vendor_id);
void kinesixd_device_free(KinesixdDevice device);
int kinesixd_device_equals(KinesixdDevice device1, KinesixdDevice device2);
const char *kinesixd_device_get_path(KinesixdDevice device);
int kinesixd_device_list_get_length(KinesixdDevice *device_list);
int kinesixd_device_list_contains(KinesixdDevice *device_list, KinesixdDevice device);
void kinesixd_device_list_free(KinesixdDevice *device_list);

#endif // DEVICE_H
