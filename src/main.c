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

#include <stdio.h>

#include <unistd.h>
#include <signal.h>

#include "kinesixd_global.h"
#include "kinesixd_dbus_adaptor.h"

static KinesixdDBusAdaptor s_dbus_adaptor = 0;

static void terminate_handler(int signo)
{
    if (signo != SIGTERM)
    {
        LOG_WARN("Caught unexpected signal %d", signo);
        return;
    }

    kinesixd_dbus_adaptor_free(s_dbus_adaptor);
    exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
    if (signal(SIGTERM, &terminate_handler) == SIG_ERR)
        LOG_ERROR("Could not set up signal handling. Closing application will end in incorrrect shutdown");

    KinesixdDBusAdaptor dbus_adaptor = kinesixd_dbus_adaptor_new(DBUS_BUS_SESSION);
    s_dbus_adaptor = dbus_adaptor;
    kinesixd_dbus_adaptor_start_listenting(dbus_adaptor);

    for (;;)
        sleep(1);

    return 0;
}
