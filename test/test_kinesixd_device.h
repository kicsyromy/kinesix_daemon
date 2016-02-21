#ifndef TEST_KINESIXD_DEVICE_H
#define TEST_KINESIXD_DEVICE_H

#include "tinytest.h"
#include "kinesixd_device.h"
#include "kinesixd_device_p.h"

const char DEVICE_PATH[]            = "/dev/input/event0";
const char DEVICE_INVALID_PATH[]    = "/some/random/path";
const char DEVICE_NAME[]            = "Some Device";
const int  DEVICE_PRODUCT_ID        = 41;
const int  DEVICE_VENDOR_ID         = 43;

KinesixdDevice tested_device;

void test_kinesixd_device_new_invalid_path()
{
    tested_device = kinesixd_device_new(DEVICE_INVALID_PATH,
                                        DEVICE_NAME,
                                        DEVICE_PRODUCT_ID,
                                        DEVICE_VENDOR_ID);

    ASSERT("Device was created with an invalid path", tested_device == 0);
}

void test_kinesixd_device_new()
{
    tested_device = kinesixd_device_new(DEVICE_PATH,
                                        DEVICE_NAME,
                                        DEVICE_PRODUCT_ID,
                                        DEVICE_VENDOR_ID);

    ASSERT("Device was not created with valid path", tested_device != 0);
    ASSERT("Device path did not match passed path", (strcmp(tested_device->path, DEVICE_PATH)) == 0);
    ASSERT("Device name did not match passed name", (strcmp(tested_device->name, DEVICE_NAME)) == 0);
}

void test_kinesix_device_all()
{
    RUN(test_kinesixd_device_new_invalid_path);
    RUN(test_kinesixd_device_new);
}

#endif // TEST_KINESIXD_DEVICE_H
