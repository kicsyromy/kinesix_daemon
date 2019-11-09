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

#include "kinesixd_device.h"
#include "kinesixd_device_p.h"

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* FIXME: Not thread safe */
static unsigned long last_assigned_id = 0;

KinesixdDevice kinesixd_device_new(const char *path,
                  const char *name,
                  uint32_t product_id,
                  uint32_t vendor_id)
{
    return device_priv_new_with_id(++last_assigned_id,
                                   path,
                                   name,
                                   product_id,
                                   vendor_id);
}

struct _KinesixdDevice *device_priv_new_with_id(int id,
                                        const char *path,
                                        const char *name,
                                        uint32_t product_id,
                                        uint32_t vendor_id)
{
    KinesixdDevice self = 0;

    /* Check if file exists */
    if (access(path, F_OK) != -1)
    {
        struct stat sb;

        if (lstat(path, &sb) != -1)
        {
            /* Check if the file is a character device */
            if ((sb.st_mode & S_IFMT) == S_IFCHR)
            {
                self = (KinesixdDevice)malloc(sizeof(struct _KinesixdDevice));
                self->id = id;
                self->path = (char *)malloc(strlen(path) + 1);
                strcpy(self->path, path);
                self->name = strdup(name);
                self->product_id = product_id;
                self->vendor_id = vendor_id;
            }
        }
    }

    return self;
}

void kinesixd_device_free(KinesixdDevice self)
{
    free(self->path);
    free(self->name);
    free(self);
}

const char *kinesixd_device_get_path(KinesixdDevice self)
{
    return self->path;
}

int kinesixd_device_equals(KinesixdDevice device1, KinesixdDevice device2)
{
    int retValue = 0;
    if (device1 && device2)
        retValue = (device1->id == device2->id);
    return retValue;
}

int kinesixd_device_list_get_length(KinesixdDevice *device_list)
{
    int device_count = 0;
    KinesixdDevice current_device = 0;

    for (;;)
    {
        current_device = device_list[device_count];
        if (!current_device)
            break;
        ++device_count;
    }

    return device_count;
}

int kinesixd_device_list_contains(KinesixdDevice *device_list, KinesixdDevice device)
{
    int contains = 0;
    int device_count = 0;
    KinesixdDevice current_device = 0;

    if (device_list && device)
    {
        for (;;)
        {
            current_device = device_list[device_count];
            if (!current_device || (contains = kinesixd_device_equals(device, current_device)))
                break;
            ++device_count;
        }
    }

    return contains;
}

void kinesixd_device_list_free(KinesixdDevice *device_list)
{
    int device_count = 0;
    KinesixdDevice current_device = 0;

    for (;;)
    {
        current_device = device_list[device_count];
        if (!current_device)
            break;
        kinesixd_device_free(current_device);
        ++device_count;
    }

    free(device_list);
}
