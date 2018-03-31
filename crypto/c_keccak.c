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
#include <stdint.h>
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

void KeccakP1600_Permute_24rounds(UINT64 *state)
{
	declareABCDE

	copyFromState(A, state)
	rounds
}

void keccak(const uint8_t *in, uint8_t *md)
{
	//========================
	typedef union
	{
		uint64_t st[25];
		uint8_t temp[144];
	} state_u;

	//========================
	state_u state;

	//========================
	memset(state.st, 0, sizeof(state_u));
	memcpy(state.temp, in, 76);

	//========================
	state.temp[76] = 1;
	state.temp[135] = 128;

	//========================
	KeccakP1600_Permute_24rounds(state.st);

	//========================
	memcpy(md, state.st, 200);
}