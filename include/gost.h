/*
 * Implementation of crypto algorithm GOST 28147
 *
 * Author Andrey Mitrofanov <avmwww@gmail.com>
 *
 */
#ifndef _GOST_H_
#define _GOST_H_

typedef unsigned long word32;
typedef unsigned char word8;

void gostcrypt(word32 const *in, word8 const *k81,
               word32 *out, word32 const *key);

void gostdecrypt(word32 const *in, word8 const *k81,
                 word32 *out, word32 const *key);

void kboxinit(unsigned char *k81, unsigned char const *subst_table);

void gostcryptbuf(word32 const *in, word8 const *k81,
                  word32 *out, word32 const *key, unsigned int len);

void gostdecryptbuf(word32 const *in, word8 const *k81,
                    word32 *out, word32 const *key, unsigned int len);


/* Number of shorts in crypt block */
#define CRYPT_BLOCK_LEN			4

#endif


