/*
 * Copyright (C) 2013 Red Hat
 *
 * This file is part of GnuTLS.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <drbg-aes.h>
#include <fips.h>

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <nettle/aes.h>
#include <nettle/memxor.h>
#include <locks.h>
#include <rnd-common.h>

/* This provides a random generator for gnutls. It uses
 * three instances of the DRBG-AES-CTR generator, one for
 * each level of randomness. It uses /dev/urandom for their
 * seeding.
 */

#define RND_LOCK if (gnutls_mutex_lock(&rnd_mutex)!=0) abort()
#define RND_UNLOCK if (gnutls_mutex_unlock(&rnd_mutex)!=0) abort()

static void *rnd_mutex;

struct fips_ctx {
	struct drbg_aes_ctx nonce_context;
	struct drbg_aes_ctx normal_context;
	struct drbg_aes_ctx strong_context;
#ifdef HAVE_GETPID
	pid_t pid;
#endif
};

static int _rngfips_ctx_reinit(struct fips_ctx *fctx);
static int _rngfips_ctx_init(struct fips_ctx *fctx);

static int get_random(struct drbg_aes_ctx *ctx, struct fips_ctx *fctx,
		      void *buffer, size_t length)
{
	int ret;

	if (ctx->reseed_counter > DRBG_AES_RESEED_TIME
#ifdef HAVE_GETPID
	    || fctx->pid != getpid()
#endif
	    ) {

		ret = _rngfips_ctx_reinit(fctx);
		if (ret < 0)
			return gnutls_assert_val(ret);
	}

	ret = drbg_aes_random(ctx, length, buffer);
	if (ret == 0)
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);

	return 0;
}

static int _rngfips_ctx_init(struct fips_ctx *fctx)
{
	int ret;

	/* strong */
	ret = drbg_init(&fctx->strong_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* normal */
	ret = drbg_init(&fctx->normal_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* nonce */
	ret = drbg_init(&fctx->nonce_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

#ifdef HAVE_GETPID
	fctx->pid = getpid();
#endif
	return 0;
}

static int _rngfips_ctx_reinit(struct fips_ctx *fctx)
{
	int ret;

	/* strong */
	ret = drbg_reseed(&fctx->strong_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* normal */
	ret = drbg_reseed(&fctx->normal_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* nonce */
	ret = drbg_reseed(&fctx->nonce_context);
	if (ret < 0)
		return gnutls_assert_val(ret);

#ifdef HAVE_GETPID
	fctx->pid = getpid();
#endif
	return 0;
}

/* Initialize this random subsystem. */
static int _rngfips_init(void **_ctx)
{
/* Basic initialization is required to initialize mutexes and
   do a few checks on the implementation.  */
	struct fips_ctx *ctx;
	int ret;

	ret = _rnd_system_entropy_init();
	if (ret < 0)
		return gnutls_assert_val(ret);

	ctx = gnutls_calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

	ret = gnutls_mutex_init(&rnd_mutex);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _rngfips_ctx_init(ctx);
	if (ret < 0)
		return gnutls_assert_val(ret);

	*_ctx = ctx;

	return 0;
}

static int _rngfips_rnd(void *_ctx, int level, void *buffer, size_t length)
{
	struct fips_ctx *ctx = _ctx;
	int ret;

	RND_LOCK;
	switch (level) {
	case GNUTLS_RND_RANDOM:
		ret = get_random(&ctx->normal_context, ctx, buffer, length);
	case GNUTLS_RND_KEY:
		ret = get_random(&ctx->strong_context, ctx, buffer, length);
	default:
		ret = get_random(&ctx->nonce_context, ctx, buffer, length);
	}
	RND_UNLOCK;

	return ret;
}

static void _rngfips_deinit(void *_ctx)
{
	struct fips_ctx *ctx = _ctx;

	gnutls_mutex_deinit(&rnd_mutex);
	rnd_mutex = NULL;

	zeroize_key(ctx, sizeof(*ctx));
	free(ctx);
}

static void _rngfips_refresh(void *_ctx)
{
	/* this is predictable RNG. Don't refresh */
	return;
}

static int selftest_kat(void)
{
	int ret;

	RND_LOCK;
	ret = drbg_aes_self_test();
	RND_UNLOCK;

	if (ret == 0) {
		_gnutls_debug_log("DRBG-AES self test failed\n");
		return gnutls_assert_val(GNUTLS_E_RANDOM_FAILED);
	} else
		_gnutls_debug_log("DRBG-AES self test succeeded\n");

	return 0;
}

int crypto_rnd_prio = INT_MAX;

gnutls_crypto_rnd_st _gnutls_rnd_ops = {
	.init = _rngfips_init,
	.deinit = _rngfips_deinit,
	.rnd = _rngfips_rnd,
	.rnd_refresh = _rngfips_refresh,
	.self_test = selftest_kat,
};
