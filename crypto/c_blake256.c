#include <string.h>
#include <stdio.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;


#define U32TO8(p, v)					\
  (p)[0] = (u8)((v) >> 24); (p)[1] = (u8)((v) >> 16);	\
  (p)[2] = (u8)((v) >>  8); (p)[3] = (u8)((v)      ); 

typedef struct {
	u32 h[8];
	u8  buf[64];
} state;

const u8 padding[] =
{ 0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };



static void blake256_compress(u32* h, const u8 * datablock, const u32 key)
{
	__m128i buf1, buf2;
	
	//=======================================================================================
	//const __m128i r8 = _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1);
	static const __m128i r8 = { {1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12 } };
	
	//const __m128i r16 = _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);
	static const __m128i r16 = { { 2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13 } };

	//const __m128i u8to32 = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
	static __m128i u8to32 = { { 3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12 } };

	//=======================================================================================
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

	const __m128i m0 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 00)), u8to32);
	const __m128i m1 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 16)), u8to32);
	const __m128i m2 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 32)), u8to32);
	const __m128i m3 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 48)), u8to32);

	__m128i row1 = _mm_set_epi32(h[3], h[2], h[1], h[0]);
	__m128i row2 = _mm_set_epi32(h[7], h[6], h[5], h[4]);

	__m128i row3 = _mm_set_epi32(0x03707344, 0x13198A2E, 0x85A308D3, 0x243F6A88);
	__m128i row4 = _mm_set_epi32(0xEC4E6C89, 0x082EFA98, 0x299F31D0 ^ key, 0xA4093822 ^ key);

#include "c_blake256.rounds.sse41.h"

	tmp0 = _mm_load_si128((__m128i*)h);
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row1, row3));
	_mm_store_si128((__m128i*)h, tmp0);

	tmp0 = _mm_load_si128((__m128i*)&(h[4]));
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row2, row4));
	_mm_store_si128((__m128i*)&(h[4]), tmp0);
}


void blake256_init(state *S) 
{
	static const u32 hs[8] = {
		0x6A09E667,
		0xBB67AE85,
		0x3C6EF372,
		0xA54FF53A,
		0x510E527F,
		0x9B05688C,
		0x1F83D9AB,
		0x5BE0CD19,
	};

	memcpy(S->h, hs, sizeof(hs));
}


void blake256_update1600(state *S, const u8 *data)
{
	blake256_compress(S->h, &data[0], 512);
	blake256_compress(S->h, &data[64], 1024);
	blake256_compress(S->h, &data[128], 1536);
	memcpy((void*)(S->buf), (void*)&data[192], 8);
}

void blake256_final(state *S, u8 *digest) 
{
	static const u8 zo = 0x01;
	static const u8 msglen[8] = 
	{
		0, 0, 0, 0, 0, 0, 6, 64
	};

	memcpy(&S->buf[8], padding, 47);
	memcpy(&S->buf[55], &zo, 1);
	memcpy(&S->buf[56], msglen, 8);
	
	blake256_compress(S->h, S->buf, 1600);

	U32TO8(digest + 0, S->h[0]);
	U32TO8(digest + 4, S->h[1]);
	U32TO8(digest + 8, S->h[2]);
	U32TO8(digest + 12, S->h[3]);
	U32TO8(digest + 16, S->h[4]);
	U32TO8(digest + 20, S->h[5]);
	U32TO8(digest + 24, S->h[6]);
	U32TO8(digest + 28, S->h[7]);
}


void blake256_hash(u8 *out, const u8 *in)
{
	state S;
	blake256_init(&S);
	blake256_update1600(&S, in);
	blake256_final(&S, out);
}
