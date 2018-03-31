/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
#include "brg_endian.h"

typedef unsigned char UINT8;
typedef unsigned long long int UINT64;

#include <emmintrin.h>
//#include <emmintrin.h>
#include <tmmintrin.h>

typedef __m128i V64;
typedef __m128i V128;
typedef union {
	V128 v128;
	UINT64 v64[2];
} V6464;

#define Unrolling 24

#define ANDnu64(a, b)       _mm_andnot_si128(a, b)
#define LOAD64(a)           _mm_loadl_epi64((const V64 *)&(a))
#define CONST64(a)          _mm_loadl_epi64((const V64 *)&(a))
#define ROL64(a, o)         _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
#define STORE64(a, b)       _mm_storel_epi64((V64 *)&(a), b)
#define XOR64(a, b)         _mm_xor_si128(a, b)
#define XOReq64(a, b)       a = _mm_xor_si128(a, b)

#define ANDnu128(a, b)      _mm_andnot_si128(a, b)
#define LOAD6464(a, b)      _mm_set_epi64((__m64)(a), (__m64)(b))
#define LOAD128(a)          _mm_load_si128((const V128 *)&(a))
#define LOAD128u(a)         _mm_loadu_si128((const V128 *)&(a))
#define ROL64in128(a, o)    _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
#define STORE128(a, b)      _mm_store_si128((V128 *)&(a), b)
#define XOR128(a, b)        _mm_xor_si128(a, b)
#define XOReq128(a, b)      a = _mm_xor_si128(a, b)
#define GET64LO(a, b)       _mm_unpacklo_epi64(a, b)
#define GET64HI(a, b)       _mm_unpackhi_epi64(a, b)
#define COPY64HI2LO(a)      _mm_shuffle_epi32(a, 0xEE)
#define COPY64LO2HI(a)      _mm_shuffle_epi32(a, 0x44)
#define ZERO128()           _mm_setzero_si128()

#define GET64LOLO(a, b)     _mm_unpacklo_epi64(a, b)
#define GET64HIHI(a, b)     _mm_unpackhi_epi64(a, b)

#define SHUFFLEBYTES128(a, b)   _mm_shuffle_epi8(a, b)
#define CONST128(a)         _mm_load_si128((const V128 *)&(a))


#include "KeccakF-1600-simd128.h"
#include "KeccakP-1600-unrolling.h"

void KeccakPermutationOnWords(UINT64 *state)
{
	declareABCDE
#if (Unrolling != 24)
		unsigned int i;
#endif

	copyFromState(A, state)
		rounds
#if defined(UseMMX)
		_mm_empty();
#endif
}

void KeccakPermutationOnWordsAfterXoring(UINT64 *state, const UINT64 *input, unsigned int laneCount)
{
	declareABCDE
#if (Unrolling != 24)
		unsigned int i;
#endif
	unsigned int j;

	for (j = 0; j<laneCount; j++)
		state[j] ^= input[j];
	copyFromState(A, state)
		rounds
#if defined(UseMMX)
		_mm_empty();
#endif
}


void fromBytesToWord(UINT64 *word, const UINT8 *bytes)
{
	unsigned int i;

	*word = 0;
	for (i = 0; i<(64 / 8); i++)
		*word |= (UINT64)(bytes[i]) << (8 * i);
}


void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
	KeccakPermutationOnWordsAfterXoring((UINT64*)state, (const UINT64*)data, laneCount);
#else
	UINT64 dataAsWords[25];
	unsigned int i;

	for (i = 0; i<laneCount; i++)
		fromBytesToWord(dataAsWords + i, data + (i * 8));
	KeccakPermutationOnWordsAfterXoring((UINT64*)state, dataAsWords, laneCount);
#endif
}

void fromWordToBytes(UINT8 *bytes, const UINT64 word)
{
	unsigned int i;

	for (i = 0; i<(64 / 8); i++)
		bytes[i] = (word >> (8 * i)) & 0xFF;
}

void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
	memcpy(data, state, laneCount * 8);
#else
	unsigned int i;

	for (i = 0; i<laneCount; i++)
		fromWordToBytes(data + (i * 8), ((const UINT64*)state)[i]);
#endif
#ifdef UseBebigokimisa
	if (laneCount > 1) {
		((UINT64*)data)[1] = ~((UINT64*)data)[1];
		if (laneCount > 2) {
			((UINT64*)data)[2] = ~((UINT64*)data)[2];
			if (laneCount > 8) {
				((UINT64*)data)[8] = ~((UINT64*)data)[8];
				if (laneCount > 12) {
					((UINT64*)data)[12] = ~((UINT64*)data)[12];
					if (laneCount > 17) {
						((UINT64*)data)[17] = ~((UINT64*)data)[17];
						if (laneCount > 20) {
							((UINT64*)data)[20] = ~((UINT64*)data)[20];
						}
					}
				}
			}
		}
	}
#endif
}
