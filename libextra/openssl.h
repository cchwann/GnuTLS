/*
 * Copyright (c) 2002 Andrew McDonald <andrew@mcdonald.org.uk>
 *
 * GNUTLS-EXTRA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * GNUTLS-EXTRA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* FIXME FIXME FIXME
   Things to fix:
   error handling
   SSL->options
*/

#ifndef GNUTLS_OPENSSL_H
#define GNUTLS_OPENSSL_H
#include <gnutls/gnutls.h>
#include <gcrypt.h>

#define OPENSSL_VERSION_NUMBER (0x0090604F)
#define SSLEAY_VERSION_NUMBER OPENSSL_VERSION_NUMBER

#define SSL_ERROR_NONE        (0)
#define SSL_ERROR_SSL         (1)
#define SSL_ERROR_WANT_READ   (2)
#define SSL_ERROR_WANT_WRITE  (3)
#define SSL_ERROR_SYSCALL     (5)
#define SSL_ERROR_ZERO_RETURN (6)

#define SSL_FILETYPE_PEM (GNUTLS_X509_FMT_PEM)

#define SSL_VERIFY_NONE (0)

#define SSL_ST_OK (1)

#define X509_V_ERR_CERT_NOT_YET_VALID          (1)
#define X509_V_ERR_CERT_HAS_EXPIRED            (2)
#define X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT (3)

#define SSL_OP_ALL (0x000FFFFF)
#define SSL_OP_NO_TLSv1 (0x0400000)

typedef gnutls_x509_dn X509_NAME;
typedef gnutls_datum X509;

struct _SSL;

typedef struct
{
    int *protocol_priority;
    int *cipher_priority;
    int *comp_priority;
    int *kx_priority;
    int *mac_priority;
} SSL_METHOD;


typedef struct
{
    struct _SSL *ssl;
    int error;
    gnutls_datum *cert_list;
#define current_cert cert_list
} X509_STORE_CTX;

typedef struct _SSL_CTX
{
    SSL_METHOD *method;
    char *certfile;
    int certfile_type;
    char *keyfile;
    int keyfile_type;
    unsigned long options;

    int (*verify_callback)(int, X509_STORE_CTX *);
    int verify_mode;

} SSL_CTX;

typedef struct _SSL
{
    GNUTLS_STATE gnutls_state;
#define rbio gnutls_state
    GNUTLS_CERTIFICATE_CLIENT_CREDENTIALS gnutls_cred;
    int last_error;
    int shutdown;
    int state;
    unsigned long options;

    int (*verify_callback)(int, X509_STORE_CTX *);
    int verify_mode;
} SSL;

typedef struct
{
    GNUTLS_Version version;
    GNUTLS_BulkCipherAlgorithm cipher;
    GNUTLS_KXAlgorithm kx;
    GNUTLS_MACAlgorithm mac;
    GNUTLS_CompressionMethod compression;
    GNUTLS_CertificateType cert;
} SSL_CIPHER;

typedef struct
{
    GCRY_MD_HD handle;
} MD_CTX;

#define MD5_CTX MD_CTX
#define RIPEMD160_CTX MD_CTX

#define OpenSSL_add_ssl_algorithms()  SSL_library_init()
#define SSLeay_add_ssl_algorithms()   SSL_library_init()
#define SSLeay_add_all_algorithms()   OpenSSL_add_all_algorithms()

#define SSL_get_cipher_name(ssl) SSL_CIPHER_get_name(SSL_get_current_cipher(ssl))
#define SSL_get_cipher(ssl) SSL_get_cipher_name(ssl)
#define SSL_get_cipher_bits(ssl,bp) SSL_CIPHER_get_bits(SSL_get_current_cipher(ssl),(bp))
#define SSL_get_cipher_version(ssl) SSL_CIPHER_get_version(SSL_get_current_cipher(ssl))


/* Library initialisation functions */

int SSL_library_init(void);
void OpenSSL_add_all_algorithms(void);


/* SSL_CTX structure handling */

SSL_CTX *SSL_CTX_new(SSL_METHOD *method);
void SSL_CTX_free(SSL_CTX *ctx);
int SSL_CTX_set_default_verify_paths(SSL_CTX *ctx);
int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *certfile, int type);
int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *keyfile, int type);
void SSL_CTX_set_verify(SSL_CTX *ctx, int verify_mode,
                        int (*verify_callback)(int, X509_STORE_CTX *));
unsigned long SSL_CTX_set_options(SSL_CTX *ssl, unsigned long options);


/* SSL structure handling */

SSL *SSL_new(SSL_CTX *ctx);
void SSL_free(SSL *ssl);
void SSL_load_error_strings(void);
int SSL_get_error(SSL *ssl, int ret);
int SSL_set_fd(SSL *ssl, int fd);
void SSL_set_connect_state(SSL *ssl);
int SSL_pending(SSL *ssl);
void SSL_set_verify(SSL *ssl, int verify_mode,
                    int (*verify_callback)(int, X509_STORE_CTX *));


/* SSL connection open/close/read/write functions */

int SSL_connect(SSL *ssl);
int SSL_shutdown(SSL *ssl);
int SSL_read(SSL *ssl, const void *buf, int len);
int SSL_write(SSL *ssl, const void *buf, int len);


/* SSL_METHOD functions */

SSL_METHOD *SSLv23_client_method(void);


/* SSL_CIPHER functions */

SSL_CIPHER *SSL_get_current_cipher(SSL *ssl);
const char *SSL_CIPHER_get_name(SSL_CIPHER *cipher);
int SSL_CIPHER_get_bits(SSL_CIPHER *cipher, int *bits);
const char *SSL_CIPHER_get_version(SSL_CIPHER *cipher);


/* X509 functions */

X509_NAME *X509_get_subject_name(X509 *cert);
char *X509_NAME_oneline(gnutls_x509_dn *name, char *buf, int len);


/* BIO functions */

void BIO_get_fd(GNUTLS_STATE gnutls_state, int *fd);


/* error handling */

unsigned long ERR_get_error(void);
char *ERR_error_string(unsigned long e, char *buf);


/* RAND functions */

int RAND_status(void);
void RAND_seed(const void *buf, int num);
int RAND_bytes(unsigned char *buf, int num);
const char *RAND_file_name(char *buf, size_t len);
int RAND_load_file(const char *name, long maxbytes);
int RAND_write_file(const char *name);


/* message digest functions */

void MD5_Init(MD5_CTX *ctx);
void MD5_Update(MD5_CTX *ctx, const void *buf, int len);
void MD5_Final(unsigned char *md, MD5_CTX *ctx);
unsigned char *MD5(const unsigned char *buf, unsigned long len,
                   unsigned char *md);

void RIPEMD160_Init(RIPEMD160_CTX *ctx);
void RIPEMD160_Update(RIPEMD160_CTX *ctx, const void *buf, int len);
void RIPEMD160_Final(unsigned char *md, RIPEMD160_CTX *ctx);
unsigned char *RIPEMD160(const unsigned char *buf, unsigned long len,
                         unsigned char *md);
#endif
