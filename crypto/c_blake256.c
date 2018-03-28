#include <string.h>
#include <stdio.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;


#define U8TO32(p)					\
  (((u32)((p)[0]) << 24) | ((u32)((p)[1]) << 16) |	\
   ((u32)((p)[2]) <<  8) | ((u32)((p)[3])      ))
#define U32TO8(p, v)					\
  (p)[0] = (u8)((v) >> 24); (p)[1] = (u8)((v) >> 16);	\
  (p)[2] = (u8)((v) >>  8); (p)[3] = (u8)((v)      ); 

typedef struct {
	u32 h[8], t;
	u8  buf[64];
} state;

//const u8 sigma[][16] = {
//	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
//{ 14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
//{ 11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
//{ 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
//{ 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
//{ 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 },
//{ 12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11 },
//{ 13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10 },
//{ 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5 },
//{ 10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13 ,0 },
//{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 },
//{ 14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3 },
//{ 11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4 },
//{ 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8 },
//{ 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13 },
//{ 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9 },
//{ 12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11 },
//{ 13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10 },
//{ 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5 },
//{ 10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13 ,0 } };
//
//const u32 cst[16] = {
//	0x243F6A88,0x85A308D3,0x13198A2E,0x03707344,
//	0xA4093822,0x299F31D0,0x082EFA98,0xEC4E6C89,
//	0x452821E6,0x38D01377,0xBE5466CF,0x34E90C6C,
//	0xC0AC29B7,0xC97C50DD,0x3F84D5B5,0xB5470917 };

const u8 padding[] =
{ 0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };



static int blake256_compress(state * state, const u8 * datablock) 
{
	__m128i row1, row2, row3, row4;
	__m128i buf1, buf2;
	const __m128i r8 = _mm_set_epi8(12, 15, 14, 13, 8, 11, 10, 9, 4, 7, 6, 5, 0, 3, 2, 1);
	const __m128i r16 = _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);

	u32 m[16];
	int r;
	u64 t;

	__m128i m0, m1, m2, m3;
	const __m128i u8to32 = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

	m0 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 00)), u8to32);
	m1 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 16)), u8to32);
	m2 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 32)), u8to32);
	m3 = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*)(datablock + 48)), u8to32);

	row1 = _mm_set_epi32(state->h[3], state->h[2],
		state->h[1], state->h[0]);
	row2 = _mm_set_epi32(state->h[7], state->h[6],
		state->h[5], state->h[4]);
	row3 = _mm_set_epi32(0x03707344, 0x13198A2E, 0x85A308D3, 0x243F6A88);

	row4 = _mm_set_epi32(0xEC4E6C89, 0x082EFA98, 0x299F31D0 ^ state->t, 0xA4093822 ^ state->t);

#include "c_blake256.rounds.sse41.h"

	tmp0 = _mm_load_si128((__m128i*)state->h);
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row1, row3));
	_mm_store_si128((__m128i*)state->h, tmp0);

	tmp0 = _mm_load_si128((__m128i*)&(state->h[4]));
	tmp0 = _mm_xor_si128(tmp0, _mm_xor_si128(row2, row4));
	_mm_store_si128((__m128i*)&(state->h[4]), tmp0);

	return 0;
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

	S->t = 0;
	//S->s[0] = S->s[1] = S->s[2] = S->s[3] = 0;
}


void blake256_update64(state *S, const u8 *data)
{
	static const int left = 56;
	static const int fill = 8;

	memcpy((void*)(S->buf + left), (void*)data, fill);
	S->t += 512;
	blake256_compress(S, S->buf);
}


void blake256_update1600(state *S, const u8 *data)
{
	S->t += 512;
	blake256_compress(S, data);
	data += 64;

	S->t += 512;
	blake256_compress(S, data);
	data += 64;

	S->t += 512;
	blake256_compress(S, data);
	data += 64;

	memcpy((void*)(S->buf), (void*)data, 8);
}

void blake256_update376(state *S, const u8 *data)
{
	memcpy((void*)(S->buf + 8), (void*)data, 47);
}

void blake256_update8(state *S, const u8 *data)
{
	memcpy((void*)(S->buf + 55), (void*)data, 1);
}

void blake256_final(state *S, u8 *digest) {

	u8 msglen[8], zo = 0x01, oo = 0x81;

	U32TO8(msglen + 0, 0);
	U32TO8(msglen + 4, 1600);

	S->t -= 376;
	blake256_update376(S, padding);
	blake256_update8(S, &zo);

	S->t -= 72;
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


void blake256_hash(u8 *out, const u8 *in)
{
	state S;
	blake256_init(&S);
	blake256_update1600(&S, in);
	blake256_final(&S, out);
	return 0;
}
