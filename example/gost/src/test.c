/*
 * Test of GOST crypt algorithm
 */

#include <stdlib.h>
#include <stdio.h>

#include "gost.h"

static unsigned char const subst_table[128] = {
	1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12,
	13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12,
	4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14,
	6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2,
	7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3,
	5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11,
	14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9,
	4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3
};

/* Work substitution table */
static unsigned char k81[1024];

const word32 key[16] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	0x7777, 0x6666, 0x5555, 0x4444, 0x3333, 0x2222, 0x1111, 0
};

#define BUFFER_SAMPLES			256
static short buf_out[BUFFER_SAMPLES];
static short buf_in[BUFFER_SAMPLES];

int main(int argc, char **argv)
{
	int i;

	for (i = 0; i < BUFFER_SAMPLES; i++)
		buf_in[i] = (i << 8) | i;

	kboxinit(k81, subst_table);

	gostcryptbuf((word32 *)buf_in, k81, (word32 *)buf_out, key,
			BUFFER_SAMPLES / CRYPT_BLOCK_LEN);

	gostdecryptbuf((const word32 *)buf_out, k81,
			(word32 *)buf_out, key, BUFFER_SAMPLES / CRYPT_BLOCK_LEN);

	for (i = 0; i < BUFFER_SAMPLES; i++) {
		if (buf_out[i] != buf_in[i]) {
			printf("Crypt/decrypt mismatch: crypt 0x%04x, decrypt 0x%04x\n",
					buf_in[i], buf_out[i]);
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
}

