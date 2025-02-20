/*
 * Test of fft for tms320c55x
 */

#include <stdio.h>
#include <stdlib.h>

#include "fft.h"

#define MAX_FFT_LENGTH			256

static long fft_buf[(MAX_FFT_LENGTH + 1) * 2];

short sound_buf[MAX_FFT_LENGTH * 2];

static short fft_sin[128];

int main(int argc, char **argv)
{
	int fft_order = 8; /* fft size = 256 */

	/* prepare twiddle */
	fft_twiddle(fft_sin, fft_order + 1);

	/* forward fft */
	fft_forw(sound_buf, fft_buf, fft_sin, fft_order + 1);

	/* reverse fft */
	fft_rev(sound_buf, fft_buf, fft_sin, fft_order + 1);

	exit(EXIT_SUCCESS);
}

