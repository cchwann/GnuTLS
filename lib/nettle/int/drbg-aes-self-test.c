/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Red Hat
 * Copyright (C) 2008  Free Software Foundation, Inc.
 *  
 * The nettle library is free software; you can redistribute it and/or modify
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

/* This is a known-answer test for the DRBG-CTR-AES. 
 */

#include <config.h>
#include <drbg-aes.h>
#include <string.h>
#include <stdio.h>

struct self_test_st {
	const uint8_t entropy[DRBG_AES_SEED_SIZE];
	const char* pstring;
	const uint8_t res[4][16];
};

struct priv_st {
	struct drbg_aes_ctx *ctx;
};

/* Run a Know-Answer-Test using a dedicated test context. */
int drbg_aes_self_test(void)
{
	static const struct self_test_st tv[] = {
		 {
		  .entropy =
		 {0xb9, 0xca, 0x7f, 0xd6, 0xa0, 0xf5, 0xd3, 0x42,
		  0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
		  0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
		  0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
		  0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
		  0x00, 0x00, 0x28, 0x18, 0x00, 0x00, 0x25, 0x00},
		 .pstring = "test test test",
		 .res = {
		  {0xb9, 0x4b, 0x00, 0x40, 0x5d, 0x10, 0x58, 0x84, 
		   0x97, 0x85, 0x5d, 0x49, 0x4f, 0x66, 0x4f, 0xa3},
		  {0xe8, 0x34, 0xae, 0x3e, 0x26, 0x36, 0xe0, 0x6e, 
		   0xb7, 0x15, 0x97, 0xc3, 0xf4, 0x7c, 0x98, 0xb0},
		  {0x8b, 0xf3, 0x85, 0xe9, 0x2b, 0x7f, 0x99, 0x7f, 
		   0x76, 0x97, 0x8b, 0x60, 0x25, 0x0a, 0x80, 0x60},
		  {0x4d, 0xfd, 0x8c, 0xc0, 0x5a, 0x20, 0xc8, 0x2d,
		   0x5c, 0x33, 0x7f, 0x1c, 0x90, 0xa0, 0xe4, 0xa7}
		  }
		 },
		 {
		 .entropy = {
		  0xb9, 0xca, 0x7f, 0xd6, 0xa0, 0xf5, 0xd3, 0x42,
		  0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
		  0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
		  0x19, 0x6d, 0x84, 0x91, 0x76, 0x1c, 0x3b, 0xbe,
		  0x48, 0xb2, 0x82, 0x98, 0x68, 0xc2, 0x80, 0x00,
		  0x00, 0x00, 0x28, 0x18, 0x00, 0x00, 0x25, 0x00},
		 .pstring = "tost tost test",
		 .res = {
		  {0x12, 0x54, 0x35, 0x0f, 0x48, 0x7b, 0x6b, 0xed, 
		   0x2b, 0x3a, 0x2f, 0xa5, 0x53, 0x2c, 0x7b, 0xcb},
		  {0x39, 0x7a, 0x88, 0xd7, 0x56, 0xf2, 0xd0, 0x64, 
		   0x37, 0xa0, 0x8f, 0xf9, 0x64, 0xb2, 0x2b, 0x2b},
		  {0x4f, 0x12, 0x38, 0x71, 0x64, 0x90, 0xae, 0xb3, 
		   0xea, 0x1e, 0x01, 0xee, 0x2b, 0x03, 0x4a, 0xd3},
		  {0x9a, 0x0a, 0x9f, 0x42, 0x9b, 0x3f, 0xea, 0x63, 
		   0x84, 0xdb, 0xf2, 0x7b, 0x96, 0x38, 0x86, 0x41}
		  }
		 },
		 {
		 .entropy = {
		   0x42, 0x9c, 0x08, 0x3d, 0x82, 0xf4, 0x8a, 0x40,
		   0x66, 0xb5, 0x49, 0x27, 0xab, 0x42, 0xc7, 0xc3,
		   0x0e, 0xb7, 0x61, 0x3c, 0xfe, 0xb0, 0xbe, 0x73,
		   0xf7, 0x6e, 0x6d, 0x6f, 0x1d, 0xa3, 0x14, 0xfa,
		   0xbb, 0x4b, 0xc1, 0x0e, 0xc5, 0xfb, 0xcd, 0x46,
		   0xbe, 0x28, 0x61, 0xe7, 0x03, 0x2b, 0x37, 0x7d},
		 .pstring = "one two",
		 .res = {
		  {0x82, 0x6e, 0x20, 0xf2, 0x85, 0xc2, 0xf7, 0xc7, 
		   0x90, 0x1f, 0xff, 0x83, 0x6b, 0xaf, 0xaa, 0xd3},
		  {0x84, 0x4a, 0x39, 0x8a, 0xfe, 0xc0, 0x77, 0x30, 
		   0xf5, 0xed, 0x11, 0xa2, 0xd2, 0x20, 0xf5, 0x6c},
		  {0x83, 0x49, 0xd3, 0x41, 0x50, 0xd7, 0xc2, 0x81, 
		   0x1a, 0xdc, 0x42, 0x62, 0x49, 0xf5, 0xf9, 0x56},
		  {0xd2, 0xf2, 0x0d, 0xee, 0x9c, 0x63, 0xe6, 0xcc, 
		   0xc6, 0x41, 0x32, 0x27, 0xe0, 0xe0, 0x93, 0x85}
		  }
		 },
	};
	unsigned i, j;
	struct drbg_aes_ctx test_ctx;
	struct priv_st priv;
	int ret;
	unsigned char result[16];

	memset(&priv, 0, sizeof(priv));
	priv.ctx = &test_ctx;

	for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
		/* Setup the key.  */
		ret = drbg_aes_init(&test_ctx, DRBG_AES_SEED_SIZE, tv[i].entropy,
			strlen(tv[i].pstring), (void*)tv[i].pstring);
		if (ret == 0)
			return 0;

		if (drbg_aes_is_seeded(&test_ctx) == 0)
			return 0;

		/* Get and compare the first three results.  */
		for (j = 0; j < 3; j++) {
			/* Compute the next value.  */
			if (drbg_aes_random
			    (&test_ctx, 16, result) == 0)
				return 0;

			/* Compare it to the known value.  */
			if (memcmp(result, tv[i].res[j], 16) != 0) {
				return 0;
			}
		}

		ret = drbg_aes_reseed(&test_ctx, DRBG_AES_SEED_SIZE, tv[i].entropy,
			0, NULL);
		if (ret == 0)
			return 0;

		if (drbg_aes_random(&test_ctx, 16, result) == 0)
			return 0;

		if (memcmp(result, tv[i].res[3], 16) != 0) {
			return 0;
		}
	}

	return 1;
}
