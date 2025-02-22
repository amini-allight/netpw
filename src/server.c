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
#include "constants.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* TODO: verification of client certificate doesn't work */

struct client
{
    SSL* ssl;
    int socket;
    struct sockaddr_in addr;
    unsigned char buffer[NETPW_IO_BUFFER_SIZE];
    on_data_callback callback;
    int disconnected;
    pthread_t thread;
};

struct server
{
    SSL_CTX* ssl_context;
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
        result = SSL_read(client->ssl, client->buffer, NETPW_IO_BUFFER_SIZE);

        if (!BIO_should_retry(SSL_get_rbio(client->ssl)))
        {
            break;
        }
        else
        {
            continue;
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
    CHECK_SSL(SSL_shutdown(client->ssl), client->ssl);
    SSL_free(client->ssl);
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

        struct client* client = malloc(sizeof(struct client));

        CHECK_POINTER(client->ssl = SSL_new(server->ssl_context));
        CHECK_OK(SSL_set_fd(client->ssl, result));
        CHECK_SSL(SSL_accept(client->ssl), client->ssl);

        printf("received connection from [%s]:%i.\n", host, port);

        client->socket = result;
        client->addr = addr;
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

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    SSL_library_init();

    const SSL_METHOD* method;

    CHECK_POINTER_FATAL(method = TLS_server_method());

    CHECK_POINTER_FATAL(server->ssl_context = SSL_CTX_new(method));

    SSL_CTX_set_min_proto_version(server->ssl_context, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(server->ssl_context, 0);

    if (ca_certificate)
    {
        BIO* certificate_reader;
        CHECK_POINTER_FATAL(certificate_reader = BIO_new_mem_buf(ca_certificate, -1));

        X509* certificate_object;
        CHECK_POINTER_FATAL(certificate_object = PEM_read_bio_X509(certificate_reader, NULL, NULL, 0));
        BIO_free_all(certificate_reader);

        X509_STORE* store;
        CHECK_POINTER_FATAL(store = X509_STORE_new());

        CHECK_OK(X509_STORE_add_cert(store, certificate_object));

        SSL_CTX_set_cert_store(server->ssl_context, store);

        SSL_CTX_set_verify(server->ssl_context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

        SSL_CTX_set_verify_depth(server->ssl_context, 1);
    }

    if (certificate && private_key)
    {
        BIO* certificate_reader;
        CHECK_POINTER_FATAL(certificate_reader = BIO_new_mem_buf(certificate, -1));

        X509* certificate_object;
        CHECK_POINTER_FATAL(certificate_object = PEM_read_bio_X509(certificate_reader, NULL, NULL, 0));
        BIO_free_all(certificate_reader);

        CHECK_OK_FATAL(SSL_CTX_use_certificate(server->ssl_context, certificate_object));
        X509_free(certificate_object);

        BIO* private_key_reader;
        CHECK_POINTER_FATAL(private_key_reader = BIO_new_mem_buf(private_key, -1));

        EVP_PKEY* private_key_object;
        CHECK_POINTER_FATAL(private_key_object = PEM_read_bio_PrivateKey(private_key_reader, NULL, NULL, 0));
        BIO_free_all(private_key_reader);

        CHECK_OK_FATAL(SSL_CTX_use_PrivateKey(server->ssl_context, private_key_object));
        EVP_PKEY_free(private_key_object);

        CHECK_OK_FATAL(SSL_CTX_check_private_key(server->ssl_context));
    }

    CHECK_ERRNO_FATAL(server->socket = socket(AF_INET, SOCK_STREAM, 0));

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

        CHECK_SSL(SSL_write(client->ssl, data, size), client->ssl);
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

    SSL_CTX_free(server->ssl_context);
    free(server);
}
