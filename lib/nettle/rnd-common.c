/*
 * Copyright (C) 2010-2012 Free Software Foundation, Inc.
 * Copyright (C) 2000, 2001, 2008 Niels Möller
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/* Here are the common parts of the random generator layer. 
 * Some of this code was based on the LSH 
 * random generator (the trivia and device source functions for POSIX)
 * and modified to fit gnutls' needs. Relicenced with permission. 
 * Original author Niels Möller.
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <locks.h>
#include <gnutls_num.h>
#include <nettle/yarrow.h>
#include <errno.h>
#include <rnd-common.h>
#include <drbg-aes.h>
#include <hash-pjw-bare.h>

void _rnd_get_event(struct event_st *e)
{
	static unsigned count = 0;

	gettime(&e->now);

#ifdef HAVE_GETRUSAGE
	if (getrusage(RUSAGE_SELF, &e->rusage) < 0) {
		_gnutls_debug_log("getrusage failed: %s\n",
				  strerror(errno));
		abort();
	}
#endif

#ifdef HAVE_GETPID
	e->pid = getpid();
#endif
	e->count = count++;
	e->err = errno;

	return;
}

#ifdef _WIN32

#include <windows.h>
#include <wincrypt.h>

static HCRYPTPROV device_fd = 0;

int _rnd_get_system_entropy_win32(void* rnd, size_t size)
{
	if (!CryptGenRandom(device_fd, (DWORD) size, rnd)) {
		_gnutls_debug_log("Error in CryptGenRandom: %s\n",
					GetLastError());
		return GNUTLS_E_RANDOM_DEVICE_ERROR;
	}

	return 0;
}

get_entropy_func _rnd_get_system_entropy = _rnd_get_system_entropy_win32;

int _rnd_system_entropy_init(void)
{
	int old;

	if (!CryptAcquireContext
		(&device_fd, NULL, NULL, PROV_RSA_FULL,
		 CRYPT_SILENT | CRYPT_VERIFYCONTEXT)) {
		_gnutls_debug_log
			("error in CryptAcquireContext!\n");
		return GNUTLS_E_RANDOM_DEVICE_ERROR;
	}
	
	return 0;
}

void _rnd_system_entropy_deinit(void)
{
	CryptReleaseContext(device_fd, 0);
}

#else /* POSIX */

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <locks.h>
#include "egd.h"

static int device_fd;

static int _rnd_get_system_entropy_urandom(void* _rnd, size_t size)
{
	uint8_t* rnd = _rnd;
	uint32_t done;

	for (done = 0; done < size;) {
		int res;
		do {
			res = read(device_fd, rnd + done, size - done);
		} while (res < 0 && errno == EINTR);

		if (res <= 0) {
			if (res < 0) {
				_gnutls_debug_log
					("Failed to read /dev/urandom: %s\n",
					 strerror(errno));
			} else {
				_gnutls_debug_log
					("Failed to read /dev/urandom: end of file\n");
			}

			return GNUTLS_E_RANDOM_DEVICE_ERROR;
		}

		done += res;
	}

	return 0;
}

static
int _rnd_get_system_entropy_egd(void* _rnd, size_t size)
{
	unsigned int done;
	uint8_t* rnd = _rnd;
	int res;

	for (done = 0; done < size;) {
		res =
		    _rndegd_read(&device_fd, rnd + done, size - done);
		if (res <= 0) {
			if (res < 0) {
				_gnutls_debug_log("Failed to read egd.\n");
			} else {
				_gnutls_debug_log("Failed to read egd: end of file\n");
			}

			return gnutls_assert_val(GNUTLS_E_RANDOM_DEVICE_ERROR);
		}
		done += res;
	}

	return 0;
}

typedef int (*get_entropy_func)(void* rnd, size_t size);

get_entropy_func _rnd_get_system_entropy = NULL;

int _rnd_system_entropy_init(void)
{
int old;
	
	device_fd = open("/dev/urandom", O_RDONLY);
	if (device_fd < 0) {
		_gnutls_debug_log("Cannot open urandom!\n");
		goto fallback;
	}

	old = fcntl(device_fd, F_GETFD);
	if (old != -1)
		fcntl(device_fd, F_SETFD, old | FD_CLOEXEC);

	_rnd_get_system_entropy = _rnd_get_system_entropy_urandom;

	return 0;
fallback:
	device_fd = _rndegd_connect_socket();
	if (device_fd < 0) {
		_gnutls_debug_log("Cannot open egd socket!\n");
		return
			gnutls_assert_val
			(GNUTLS_E_RANDOM_DEVICE_ERROR);
	}
	_rnd_get_system_entropy = _rnd_get_system_entropy_egd;
	
	return 0;
}

void _rnd_system_entropy_deinit(void)
{
	if (device_fd > 0) {
		close(device_fd);
		device_fd = -1;
	}
}
#endif

#define PSTRING "gnutls-rng"
#define PSTRING_SIZE (sizeof(PSTRING)-1)
int drbg_init(struct drbg_aes_ctx *ctx)
{
	uint8_t buffer[DRBG_AES_SEED_SIZE];
	int ret;

	/* Get a key from the standard RNG or from the entropy source.  */
	ret = _rnd_get_system_entropy(buffer, sizeof(buffer));
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = drbg_aes_init(ctx, sizeof(buffer), buffer, PSTRING_SIZE, (void*)PSTRING);
	if (ret == 0)
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);

	zeroize_key(buffer, sizeof(buffer));

	return 0;
}

/* Reseed a generator. */
int drbg_reseed(struct drbg_aes_ctx *ctx)
{
	uint8_t buffer[DRBG_AES_SEED_SIZE];
	int ret;

	/* The other two generators are seeded from /dev/random.  */
	ret = _rnd_get_system_entropy(buffer, sizeof(buffer));
	if (ret < 0)
		return gnutls_assert_val(ret);

	drbg_aes_reseed(ctx, sizeof(buffer), buffer, 0, NULL);

	return 0;
}
