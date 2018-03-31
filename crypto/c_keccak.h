// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

#ifndef KECCAK_H
#define KECCAK_H

#include <stdint.h>
#include <string.h>

#ifndef KECCAK_ROUNDS
#define KECCAK_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

// update the state
void keccakf(uint64_t st[25], int norounds);
void KeccakP1600_Permute_24rounds(uint64_t *state);

// compute a keccak hash (md) of given byte length from "in"
void keccak(const uint8_t *in, uint8_t *md);
void keccakV(const uint8_t *in, int inlen, uint8_t *md, int mdlen);

#endif
