/*
Copyright 2025 Amini Allight

This file is part of netpw.

netpw is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

netpw is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with netpw. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef NETPW_CLIENT_H
#define NETPW_CLIENT_H

#include "callback.h"

struct client;

struct client* client_init(
    const char* host,
    unsigned short port,
    const char* ca_certificate,
    const char* certificate,
    const char* private_key,
    on_data_callback callback
);
void client_send(struct client* client, const unsigned char* data, int size);
void client_destroy(struct client* client);

#endif
