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
#include "client.h"
#include "error_handling.h"

#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

/* TODO: Network security */

struct client
{
    int socket;
    unsigned char buffer[BUFFER_SIZE];
    on_data_callback callback;
    pthread_t thread;
};

static void* client_receive(void* arg)
{
    struct client* client = arg;

    int result;

    while (1)
    {
        result = recv(client->socket, client->buffer, BUFFER_SIZE, 0);

        if (result <= 0)
        {
            break;
        }

        client->callback(client->buffer, result);
    }

    printf("disconnected.\n");

    return NULL;
}

struct client* client_init(
    const char* host,
    unsigned short port,
    const char* ca_certificate,
    const char* certificate,
    const char* private_key,
    on_data_callback callback
)
{
    int result;

    struct client* client = malloc(sizeof(struct client));

    CHECK_ERRNO_FATAL(client->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

    struct addrinfo* info = NULL;

    char port_string[6] = {0};
    snprintf(port_string, 6, "%i", port);

    CHECK_ERROR_FATAL(getaddrinfo(host, port_string, NULL, &info));

    CHECK_ERRNO_FATAL(connect(client->socket, info->ai_addr, info->ai_addrlen));

    freeaddrinfo(info);

    printf("connected to [%s]:%i.\n", host, port);

    client->callback = callback;

    CHECK_ERROR_FATAL(pthread_create(&client->thread, NULL, client_receive, client));

    return client;
}

void client_send(struct client* client, const unsigned char* data, int size)
{
    int result;

    CHECK_ERRNO(send(client->socket, data, size, 0));
}

void client_destroy(struct client* client)
{
    int result;

    CHECK_ERRNO(close(client->socket));
    CHECK_ERROR(pthread_join(client->thread, NULL));
    free(client);
}
