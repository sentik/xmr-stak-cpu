#ifndef _BLAKE256_H_
#define _BLAKE256_H_

#include <stdint.h>
void blake256_hash(uint8_t *, const uint8_t *);
void blake256_hash2(uint8_t *out, const uint8_t *in, uint64_t inlen);

#endif /* _BLAKE256_H_ */
