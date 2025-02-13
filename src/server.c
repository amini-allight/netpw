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
#include "server.h"
#include "error_handling.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 4096

struct client
{
    struct sockaddr_in addr;
    int socket;
    unsigned char buffer[BUFFER_SIZE];
    on_data_callback callback;
    int disconnected;
    pthread_t thread;
};

struct server
{
    int socket;
    struct client** clients;
    int client_count;
    sem_t client_lock;
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

    char host[INET6_ADDRSTRLEN] = {0};
    inet_ntop(client->addr.sin_family, &client->addr.sin_addr, host, INET6_ADDRSTRLEN);
    unsigned short port = ntohs(client->addr.sin_port);

    printf("connection closed to [%s]:%i.\n", host, port);
    client->disconnected = 1;

    return NULL;
}

static void client_destroy(struct client* client)
{
    int result;

    CHECK_ERRNO(close(client->socket));
    CHECK_ERROR(pthread_join(client->thread, NULL));
    free(client);
}

static void* server_accept(void* arg)
{
    struct server* server = arg;

    int result;

    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);

        CHECK_ERRNO(accept(server->socket, (struct sockaddr*)&addr, &addr_size));

        if (result < 0)
        {
            break;
        }

        char host[INET6_ADDRSTRLEN] = {0};
        inet_ntop(addr.sin_family, &addr.sin_addr, host, INET6_ADDRSTRLEN);
        unsigned short port = ntohs(addr.sin_port);

        printf("received connection from [%s]:%i.\n", host, port);

        struct client* client = malloc(sizeof(struct client));

        client->addr = addr;
        client->socket = result;
        client->callback = server->callback;
        client->disconnected = 0;
        CHECK_ERROR(pthread_create(&client->thread, NULL, client_receive, client));

        /* remove dead clients */
        CHECK_ERRNO(sem_wait(&server->client_lock));

        int i;
        for (i = 0; i < server->client_count;)
        {
            if (server->clients[i]->disconnected)
            {
                int next_index = i + 1;
                memcpy(server->clients + i, server->clients + next_index, (server->client_count - next_index) * sizeof(struct client*));
                server->client_count--;
            }
            else
            {
                i++;
            }
        }

        CHECK_ERRNO(sem_post(&server->client_lock));

        /* add new client */
        int next_client_count = server->client_count + 1;
        struct client** next_clients = malloc(next_client_count * sizeof(struct client*));

        memcpy(next_clients, server->clients, server->client_count * sizeof(struct client*));
        next_clients[server->client_count] = client;

        /* replace clients */
        CHECK_ERRNO(sem_wait(&server->client_lock));

        free(server->clients);
        server->clients = next_clients;
        server->client_count = next_client_count;

        CHECK_ERRNO(sem_post(&server->client_lock));
    }

    return NULL;
}

struct server* server_init(
    const char* host,
    unsigned short port,
    const char* ca_certificate,
    const char* certificate,
    const char* private_key,
    on_data_callback callback
)
{
    int result;

    struct server* server = malloc(sizeof(struct server));

    CHECK_ERRNO_FATAL(server->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

    struct addrinfo* info = NULL;

    char port_string[6] = {0};
    snprintf(port_string, 6, "%i", port);

    CHECK_ERROR_FATAL(getaddrinfo(host, port_string, NULL, &info));

    CHECK_ERRNO_FATAL(bind(server->socket, info->ai_addr, info->ai_addrlen));

    freeaddrinfo(info);

    CHECK_ERRNO_FATAL(listen(server->socket, 1));

    server->clients = NULL;
    server->client_count = 0;
    CHECK_ERRNO_FATAL(sem_init(&server->client_lock, 0, 1));
    server->callback = callback;

    CHECK_ERROR_FATAL(pthread_create(&server->thread, NULL, server_accept, server));

    printf("listening at [%s]:%i.\n", host, port);

    return server;
}

void server_send(struct server* server, const unsigned char* data, int size)
{
    int result;

    CHECK_ERRNO(sem_wait(&server->client_lock));

    int i;
    for (i = 0; i < server->client_count; i++)
    {
        struct client* client = server->clients[i];

        if (client->disconnected)
        {
            continue;
        }

        CHECK_ERRNO(send(client->socket, data, size, 0));
    }

    CHECK_ERRNO(sem_post(&server->client_lock));
}

void server_destroy(struct server* server)
{
    int result;

    CHECK_ERRNO(close(server->socket));
    CHECK_ERROR(pthread_join(server->thread, NULL));
    CHECK_ERRNO(sem_destroy(&server->client_lock));

    int i;
    for (i = 0; i < server->client_count; i++)
    {
        client_destroy(server->clients[i]);
    }
    free(server->clients);

    free(server);
}
