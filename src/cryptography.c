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
#include "cryptography.h"
#include "constants.h"

#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

int generate_rsa_key_pair(char** public_key, char** private_key)
{
    EVP_PKEY* keypair = NULL;
    BIO* public_writer = BIO_new(BIO_s_mem());
    BIO* private_writer = BIO_new(BIO_s_mem());
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    EVP_PKEY_keygen_init(ctx);

    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 4096);

    EVP_PKEY_keygen(ctx, &keypair);

    PEM_write_bio_PUBKEY(public_writer, keypair);
    int public_key_size = BIO_pending(public_writer);
    *public_key = malloc(public_key_size);
    BIO_read(public_writer, *public_key, public_key_size);

    PEM_write_bio_PrivateKey(private_writer, keypair, NULL, NULL, 0, NULL, NULL);
    int private_key_size = BIO_pending(private_writer);
    *private_key = malloc(private_key_size);
    BIO_read(private_writer, private_key, private_key_size);

    EVP_PKEY_free(keypair);
    EVP_PKEY_CTX_free(ctx);
    BIO_free_all(private_writer);
    BIO_free_all(public_writer);

    return 0;
}

int x509_certificate_from_private_key(const char* private_key, char** certificate)
{
    X509* cert = X509_new();
    BIO* cert_writer = BIO_new(BIO_s_mem());
    BIO* key_reader = BIO_new_mem_buf(private_key, -1);

    EVP_PKEY* key;
    key = PEM_read_bio_PrivateKey(key_reader, NULL, NULL, 0);

    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 315360000);

    X509_set_pubkey(cert, key);

    X509_NAME* name = X509_get_subject_name(cert);

    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)NETPW_PROGRAM_NAME, -1, -1, 0);

    X509_set_issuer_name(cert, name);

    X509_sign(cert, key, EVP_sha256());

    PEM_write_bio_X509(cert_writer, cert);

    int certificate_size = BIO_pending(cert_writer);
    *certificate = malloc(certificate_size);
    BIO_read(cert_writer, *certificate, certificate_size);

    EVP_PKEY_free(key);
    BIO_free_all(key_reader);
    BIO_free_all(cert_writer);
    X509_free(cert);

    return 0;
}
