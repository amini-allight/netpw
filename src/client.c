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
#include "constants.h"

#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

struct client
{
    SSL_CTX* ssl_context;
    SSL* ssl;
    int socket;
    unsigned char buffer[NETPW_IO_BUFFER_SIZE];
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

        if (result <= 0)
        {
            if (!BIO_should_retry(SSL_get_rbio(client->ssl)))
            {
                break;
            }
            else
            {
                continue;
            }
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

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    SSL_library_init();

    const SSL_METHOD* method;

    CHECK_POINTER_FATAL(method = TLS_client_method());

    CHECK_POINTER_FATAL(client->ssl_context = SSL_CTX_new(method));

    SSL_CTX_set_min_proto_version(client->ssl_context, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(client->ssl_context, 0);

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

        SSL_CTX_set_cert_store(client->ssl_context, store);

        SSL_CTX_set_verify(client->ssl_context, SSL_VERIFY_PEER, NULL);

        SSL_CTX_set_verify_depth(client->ssl_context, 1);
    }

    if (certificate && private_key)
    {
        BIO* certificate_reader;
        CHECK_POINTER_FATAL(certificate_reader = BIO_new_mem_buf(certificate, -1));

        X509* certificate_object;
        CHECK_POINTER_FATAL(certificate_object = PEM_read_bio_X509(certificate_reader, NULL, NULL, 0));
        BIO_free_all(certificate_reader);

        CHECK_OK_FATAL(SSL_CTX_use_certificate(client->ssl_context, certificate_object));
        X509_free(certificate_object);

        BIO* private_key_reader;
        CHECK_POINTER_FATAL(private_key_reader = BIO_new_mem_buf(private_key, -1));

        EVP_PKEY* private_key_object;
        CHECK_POINTER_FATAL(private_key_object = PEM_read_bio_PrivateKey(private_key_reader, NULL, NULL, 0));
        BIO_free_all(private_key_reader);

        CHECK_OK_FATAL(SSL_CTX_use_PrivateKey(client->ssl_context, private_key_object));
        EVP_PKEY_free(private_key_object);

        CHECK_OK_FATAL(SSL_CTX_check_private_key(client->ssl_context));
    }

    CHECK_POINTER_FATAL(client->ssl = SSL_new(client->ssl_context));

    CHECK_ERRNO_FATAL(client->socket = socket(AF_INET, SOCK_STREAM, 0));

    struct addrinfo* info = NULL;

    char port_string[6] = {0};
    snprintf(port_string, 6, "%i", port);

    CHECK_ERROR_FATAL(getaddrinfo(host, port_string, NULL, &info));
    CHECK_ERRNO_FATAL(connect(client->socket, info->ai_addr, info->ai_addrlen));
    freeaddrinfo(info);

    CHECK_OK_FATAL(SSL_set_fd(client->ssl, client->socket));
    CHECK_SSL_FATAL(SSL_connect(client->ssl), client->ssl);
    CHECK_SSL_FATAL(SSL_do_handshake(client->ssl), client->ssl);

    if (ca_certificate)
    {
        X509* remote_certificate = NULL;
        CHECK_POINTER_FATAL(remote_certificate = SSL_get1_peer_certificate(client->ssl));
        X509_free(remote_certificate);

        CHECK_ERROR_FATAL(SSL_get_verify_result(client->ssl));
    }

    printf("connected to [%s]:%i.\n", host, port);

    client->callback = callback;

    CHECK_ERROR_FATAL(pthread_create(&client->thread, NULL, client_receive, client));

    return client;
}

void client_send(struct client* client, const unsigned char* data, int size)
{
    int result;

    CHECK_SSL(SSL_write(client->ssl, data, size), client->ssl);
}

void client_destroy(struct client* client)
{
    int result;

    CHECK_ERRNO(close(client->socket));
    CHECK_ERROR(pthread_join(client->thread, NULL));
    CHECK_SSL(SSL_shutdown(client->ssl), client->ssl);
    SSL_free(client->ssl);
    SSL_CTX_free(client->ssl_context);
    free(client);
}
