#include <string.h>
#include <stdio.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

#define U32TO8(p, v)					\
  (p)[0] = (u8)((v) >> 24); (p)[1] = (u8)((v) >> 16);	\
  (p)[2] = (u8)((v) >>  8); (p)[3] = (u8)((v)      ); 

typedef struct 
{
	u32 h[8];
	u8  buf[64];
} state;

static const u8 padding[] =
{ 
	0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 
};

static const __m128i r8 = { {12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1} };
static const __m128i r16 = { {13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2} };
static const __m128i u8to32 = { {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3} };


void blake256_compress(state * state, const u8 * datablock, const u32 t) 
{
	// ReSharper disable CppJoinDeclarationAndAssignment
	__m128i buf1, buf2;
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
	// ReSharper restore CppJoinDeclarationAndAssignment

	const __m128i m0 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 00)), u8to32);
	const __m128i m1 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 16)), u8to32);
	const __m128i m2 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 32)), u8to32);
	const __m128i m3 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 48)), u8to32);

	__m128i row1 = _mm_set_epi32(state->h[3], state->h[2],	state->h[1], state->h[0]);
	__m128i row2 = _mm_set_epi32(state->h[7], state->h[6],	state->h[5], state->h[4]);
	__m128i row3 = _mm_set_epi32(0x03707344, 0x13198A2E, 0x85A308D3, 0x243F6A88);
	__m128i row4 = _mm_set_epi32(0xEC4E6C89, 0x082EFA98, 0x299F31D0 ^ t, 0xA4093822 ^ t);

#include "c_blake256.rounds.sse41.h"

	tmp0 = _mm_load_si128((__m128i*)state->h);
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row1, row3));
	_mm_store_si128((__m128i*)state->h, tmp0);

	tmp0 = _mm_load_si128((__m128i*)&(state->h[4]));
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row2, row4));
	_mm_store_si128((__m128i*)&(state->h[4]), tmp0);
}

__forceinline void blake256_init(state *S) 
{
	static const u32 ch[8] =
	{
		0x6A09E667,
		0xBB67AE85,
		0x3C6EF372,
		0xA54FF53A,
		0x510E527F,
		0x9B05688C,
		0x1F83D9AB,
		0x5BE0CD19,
	};

	memcpy(S->h, ch, sizeof(ch));
}


__forceinline void blake256_update64(state *S, const u8 *data) {

	static const int fill = 8;

	//===============================
	memcpy((void*)(S->buf + 56), (void*)data, fill);
	blake256_compress(S, S->buf, 1600);
}

__forceinline void blake256_update8(state *S, const u8 *data)
{
	memcpy((void*)(S->buf + 55), (void*)data, 8 >> 3);
}

__forceinline void blake256_update376(state *S, const u8 *data)
{
	memcpy((void*)(S->buf + 8), (void*)data, 376 >> 3);
}

__forceinline void blake256_update1600(state *S, const u8 *data)
{
	//==================================
	blake256_compress(S, data, 512);
	data += 64;

	//==================================
	blake256_compress(S, data, 1024);
	data += 64;

	//==================================
	blake256_compress(S, data, 1536);
	data += 64;

	//==================================
	memcpy((void*)(S->buf + 0), (void*)data, 64 >> 3);
}


__forceinline void blake256_final(state *S, u8 *digest) {

	u8 msglen[8], zo = 0x01;

	U32TO8(msglen + 0, 0);
	U32TO8(msglen + 4, 1600);

	blake256_update376(S, padding);
	blake256_update8(S, &zo);
	blake256_update64(S, msglen);

	U32TO8(digest + 0, S->h[0]);
	U32TO8(digest + 4, S->h[1]);
	U32TO8(digest + 8, S->h[2]);
	U32TO8(digest + 12, S->h[3]);
	U32TO8(digest + 16, S->h[4]);
	U32TO8(digest + 20, S->h[5]);
	U32TO8(digest + 24, S->h[6]);
	U32TO8(digest + 28, S->h[7]);
}


void blake256_hash(unsigned char *out, const unsigned char *in)
{
	state S;
	blake256_init(&S);
	blake256_update1600(&S, in);
	blake256_final(&S, out);
}