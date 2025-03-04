/*
 * Implementation of crypto algorithm GOST 28147
 *
 * Author Andrey Mitrofanov <avmwww@gmail.com>
 *
 */
#include "gost.h"

void gostcryptbuf(word32 const *in, word8 const *k81,
                  word32 *out, word32 const *key, unsigned int len)
{
  int i;

  for (i = 0; i < len; i++) {
    gostcrypt(in, k81, out, key);
    in += 2;
    out += 2;
  }
}

void gostdecryptbuf(word32 const *in, word8 const *k81,
                    word32 *out, word32 const *key, unsigned int len)
{
  int i;

  for (i = 0; i < len; i++) {
    gostdecrypt(in, k81, out, key);
    in += 2;
    out += 2;
  }
}

