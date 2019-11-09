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

#include "kinesixd_device_marshaler.h"

#include "kinesixd_device_p.h"

typedef enum
{
    ARG_DEV_ID = 0,
    ARG_DEV_PATH,
    ARG_DEV_NAME,
    ARG_DEV_PRODUCT_ID,
    ARG_DEV_VENDOR_ID,
    ARG_DEV_COUNT
} DeviceField;

static const char *device_dbus_type = "(issuu)";
static const int device_argument_dbus_types[] =
{
    DBUS_TYPE_INT32,
    DBUS_TYPE_STRING,
    DBUS_TYPE_STRING,
    DBUS_TYPE_UINT32,
    DBUS_TYPE_UINT32,
    DBUS_TYPE_INVALID
};

int kinesixd_device_marshaler_append_device(const KinesixdDevice device, DBusMessageIter *dbus_iter)
{
    DBusMessageIter dbus_struct;
    int device_id = 0;
    char *device_path = 0;
    char *device_name = 0;
    uint32_t device_product_id;
    uint32_t device_vendor_id;
    int error_set = 0;

    device_id = device->id;
    device_path = strdup(device->path);
    device_name = strdup(device->name);
    device_product_id = device->product_id;
    device_vendor_id = device->vendor_id;

    if ((error_set = !dbus_message_iter_open_container(dbus_iter,
                                     DBUS_TYPE_STRUCT,
                                     0,
                                     &dbus_struct)))
    {
        LOG_ERROR("Failed to open DBus container for device %s. Not enough memory", device_path);
    }

    if (!error_set && (error_set = !dbus_message_iter_append_basic(&dbus_struct, DBUS_TYPE_INT32, &device_id)))
        LOG_ERROR("Failed while trying to marshal device %s. Not enough memory", device_path);
    if (!error_set && (error_set = !dbus_message_iter_append_basic(&dbus_struct, DBUS_TYPE_STRING, &device_path)))
        LOG_ERROR("Failed while trying to marshal device %s. Not enough memory", device_path);
    if (!error_set && (error_set = !dbus_message_iter_append_basic(&dbus_struct, DBUS_TYPE_STRING, &device_name)))
        LOG_ERROR("Failed while trying to marshal device %s. Not enough memory", device_path);
    if (!error_set && (error_set = !dbus_message_iter_append_basic(&dbus_struct, DBUS_TYPE_UINT32, &device_product_id)))
        LOG_ERROR("Failed while trying to marshal device %s. Not enough memory", device_path);
    if (!error_set && (error_set = !dbus_message_iter_append_basic(&dbus_struct, DBUS_TYPE_UINT32, &device_vendor_id)))
        LOG_ERROR("Failed while trying to marshal device %s. Not enough memory", device_path);
    if (!error_set && (error_set = !dbus_message_iter_close_container(dbus_iter, &dbus_struct)))
        LOG_ERROR("Failed to close DBus container for device %s. Not enough memory", device_path);

    free(device_path);
    free(device_name);

    if (error_set)
        dbus_message_iter_abandon_container(dbus_iter, &dbus_struct);

    return error_set;
}

KinesixdDevice kinesixd_device_marshaler_device_from_dbus_argument(DBusMessageIter *dbus_iter)
{
    KinesixdDevice device = 0;
    DBusMessageIter dbus_device;
    int current_arg_type = DBUS_TYPE_INVALID;
    int expected_arg_type = DBUS_TYPE_INVALID;
    DeviceField dbus_arg_field = ARG_DEV_ID;
    int device_id = 0;
    const char *device_path = 0;
    const char *device_name = 0;
    uint32_t device_product_id = 0;
    uint32_t device_vendor_id = 0;

    if (dbus_message_iter_get_arg_type(dbus_iter) == DBUS_TYPE_STRUCT)
    {
        dbus_message_iter_recurse(dbus_iter, &dbus_device);

        while ((current_arg_type = dbus_message_iter_get_arg_type(&dbus_device)) != DBUS_TYPE_INVALID)
        {
            if (dbus_arg_field >= ARG_DEV_COUNT)
            {
                LOG_WARN("Too many arguments for Device structure."
                         "Expected %d but received at least one more. "
                         "All extra arguments ignored",
                         ARG_DEV_COUNT);
                break;
            }
            else if (current_arg_type == (expected_arg_type = device_argument_dbus_types[dbus_arg_field]))
            {
                switch(dbus_arg_field)
                {
                case ARG_DEV_ID:
                    dbus_message_iter_get_basic(&dbus_device, &device_id);
                    break;
                case ARG_DEV_PATH:
                    dbus_message_iter_get_basic(&dbus_device, &device_path);
                    break;
                case ARG_DEV_NAME:
                    dbus_message_iter_get_basic(&dbus_device, &device_name);
                    break;
                case ARG_DEV_PRODUCT_ID:
                    dbus_message_iter_get_basic(&dbus_device, &device_product_id);
                    break;
                case ARG_DEV_VENDOR_ID:
                    dbus_message_iter_get_basic(&dbus_device, &device_vendor_id);
                    break;
                default:
                    break;
                }
            }
            else
            {
                LOG_ERROR("Invalid argument received. Expecting %d but received %d",
                          current_arg_type, expected_arg_type);
            }

            ++dbus_arg_field;
            dbus_message_iter_next(&dbus_device);
        }

        if (dbus_arg_field == ARG_DEV_COUNT)
            device = device_priv_new_with_id(device_id,
                                             device_path,
                                             device_name,
                                             device_product_id,
                                             device_vendor_id);
        else
            LOG_ERROR("To few arguments for Device structure."
                      "Expected %d arguments but received %d",
                      ARG_DEV_COUNT, dbus_arg_field);
    }

    return device;
}

int kinesixd_device_marshaler_append_device_list(const KinesixdDevice *const device_list, DBusMessageIter *dbus_iter)
{
    DBusMessageIter dbus_array;
    KinesixdDevice device = 0;
    int i = 0;
    int error_set = 0;

    if ((error_set = !dbus_message_iter_open_container(dbus_iter,
                                     DBUS_TYPE_ARRAY,
                                     device_dbus_type,
                                     &dbus_array)))
    {
        LOG_ERROR("Failed to open DBus container for device list. Not enough memory");
    }
    else
    {
        for (;;)
        {
            device = device_list[i++];
            if (!device)
                break;

            if ((error_set = kinesixd_device_marshaler_append_device(device, &dbus_array)))
                break;
        }
        if (!error_set && (error_set = !dbus_message_iter_close_container(dbus_iter, &dbus_array)))
            LOG_ERROR("Failed to close DBus container for device list. Not enough memory");
    }
    if (error_set)
        dbus_message_iter_abandon_container(dbus_iter, &dbus_array);

    return error_set;
}
