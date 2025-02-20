/*
 * Implementation of Fast Fourier Transform on the platform
 * TI tms320c55x
 *
 * Author Andrey Mitrofanov <avmwww@gmail.com>
 *
 */
#ifndef _FFT_H_
#define _FFT_H_

/* pi in short format */
#define PRF_SH			32768
#define PI_SHORT		32768
#define PI_SHORT_2		65535

void fft_twiddle (short *si, short range);
void fft_forw(short *x, long *y, short *si, short range);
void fft_rev(short *x, long *y, short *si, short range);
short sin_short(short rad);
void fft_module(short *x, short range);
void fft_module_power(short *x, short range);

void sori_forward(short *x, long *y, short *si, short range);

inline short max_bit_for_forward (short range)
{
  switch (1 << range) {
    case 256:
    case 512:
      return 12;
    case 1024:
    case 2048:
      return 11;
  }
  return 11;
}

inline short max_bit_for_invers (short range)
{
  switch (1 << range) {
    case  256: return 12;
    case  512: return 11;
    case 1024: return 10;
    case 2048: return  9;
  }
  return 10;
}

#endif
