#ifndef KNTLM_DES_H
#define KNTLM_DES_H

#include <qglobal.h>

typedef struct des_key
{
  char kn[16][8];
  Q_UINT32 sp[8][64];
  char iperm[16][16][8];
  char fperm[16][16][8];
} DES_KEY;

int
ntlm_des_ecb_encrypt (const void *plaintext, int len, DES_KEY * akey, unsigned char output[8]);
int
ntlm_des_set_key (DES_KEY * dkey, char *user_key, int len);

#endif /*  KNTLM_DES_H */
