/*
 * Copyright (c) 2017  Devin Butterfield
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * Functions for encoding ALE messages
 */
 
#include <stdio.h>
#include <string.h>
#include "ale_symbol_library.h"
#include "mod.h"
#include "golay.h"

const int ale_symbol_library[ALE_SYMBOL_COUNT][ALE_SYMBOL_SIZE] = ALE_SYMBOL_LIBRARY;

/* 
 * Build array of address words from the given null terminated string.
 * type: one of the supported ALE Address type words.
 * address: null terminated address character string.
 * words: pointer to output word buffer, MUST be at least 5 integers long
 * len: pointer to int variable to store output buffer length
 * 
 * returns 0 on success, -1 on error.
 */
int build_address_words(int type, char *address, unsigned int *words, int *len)
{
	int i,j;
	int alen;
	int word;
	int next_type = ALE_WORD_TYPE_DATA;

	*len = 0;
	switch (type) {
	case ALE_WORD_TYPE_THRU:
		/* XXX FIXME: ALE THRU type not handled */
		printf("not implemented!\n");
		return -1;
	case ALE_WORD_TYPE_TO:
	case ALE_WORD_TYPE_TIS:
	case ALE_WORD_TYPE_TWAS:
	case ALE_WORD_TYPE_FROM:
		alen = strlen(address);
		if (alen > 15) {
			printf("address must be less than 15 characters!\n");
			return -1;
		}
		word = (type << 21);
		for (i = 0; i < 5; i++) {
			for (j = 2; j >= 0; j--) {
				if (*address) {
					word |= (*address << (7*j));
					address++;
				} else {
					word |= ('@' << (7*j));
				}
			}
			*words = word;
			words++;
			*len = *len + 1;

			word = (next_type << 21);
			next_type = (next_type == ALE_WORD_TYPE_DATA) ? ALE_WORD_TYPE_REP : ALE_WORD_TYPE_DATA;

			if (*address == 0) {
				break;
			}			
		}
		break;
	default:
		printf("invalid address word type %d\n", type);
		return -1;
	}
	return 0;
}

/*
 * Given an array of encoded words, and specified length, 
 * perform FEC encoding, interleaving, tripling, and modulation.
 * 
 * words: array of encoded ALE words
 * len: length of word array
 * samples: pointer to buffer big enough to store 3136 samples (for modulated triple redundant word)
 * 
 * Stores samples of modulated triple redundant word (49 symbols X 64 samples each; 3136)
 * in buffer pointed to by "samples".
 * 
 * Returns word index. Must be called with same words and len and new samples buffers until return == len.
 */
#define TESTING

int mod(unsigned int *words, int len, short *samples)
{
	unsigned int codeA;
	unsigned int codeB;
	static int word_idx = 0;
	unsigned int half_wordA;
	unsigned int half_wordB;
	int i,j,k;
	unsigned char bits[ALE_TX_WORD_LEN];
	int sym;

#ifdef TESTING
	static FILE *test_in = 0;
	static FILE *test_out_golden = 0;
	if (!test_in) {
		test_in = fopen("test_in.dat","w");
		if (test_in == NULL) {
			printf("error opening test_in.dat\n");
		}
	}
	if (!test_out_golden) {
		test_out_golden = fopen("test_out_golden.dat","w");
		if (test_out_golden == NULL) {
			printf("error opening test_out_golden.dat\n");
		}
	}

	// fprintf(test_in, "%d\n", words[word_idx]);
#endif

	/* word B from bottom 12 bits */
	half_wordB = words[word_idx] & 0xfff;
	codeB = golay_encode(half_wordB);

	/* word A from top 12 bits */
	half_wordA = (words[word_idx] >> 12) & 0xfff;
	codeA = golay_encode(half_wordA);

	codeB = codeB ^ 0xFFF;

	printf("0x%x 0x%x\n",codeA, codeB);

	/* interleave */
	j = 0;
	for (i = ALE_WORD_LEN-1; i >= 0; i--) {
		bits[j++] = (codeA >> i) & 1;
		bits[j++] = (codeB >> i) & 1;		
	}
	bits[j] = 0; // stuff bit

	/* triple the words with 1 stuff bit each and modulate into 49 symbols */
	j = 0;
	for (i = 0; i < ALE_TX_WORD_LEN; i++) {
		sym = bits[j] << 2; 
		j = (j+1)%ALE_TX_WORD_LEN;

		sym |= bits[j] << 1; 
		j = (j+1)%ALE_TX_WORD_LEN;
		
		sym |= bits[j] << 0; 
		j = (j+1)%ALE_TX_WORD_LEN;


#ifdef TESTING
		fprintf(test_in, "%d\n", sym);
#endif
// #ifdef TESTING
		// fprintf(test_out_golden, "%d\n", sym);
		// fflush(test_in);
		// fflush(test_out_golden);		
// #endif
		// memcpy(samples, ale_symbol_library[sym], ALE_SYMBOL_SIZE*sizeof(short));
		for (k=0; k<ALE_SYMBOL_SIZE;k++) {
			*samples++ = ale_symbol_library[sym][k] >> 8;
			fprintf(test_out_golden, "%d\n",ale_symbol_library[sym][k]);
		}		
		// samples+=ALE_SYMBOL_SIZE;
	}

#ifdef TESTING
	fflush(test_in);
	fflush(test_out_golden);		
#endif

	word_idx++;
	if (word_idx == len) {
		word_idx = 0;
		return len;
	}
	return word_idx;
}

/*
 * Given the specified scan time in millisenconds, whether a reponse is requested/allowed, 
 * and the specified address, generate samples for transmission. This function should be
 * called repeatedly with same first 3 args, but new samples buffer, until it returns 0.
 * Each call returns 3136 samples (one complete modulated triple-redundant word).
 * 
 * scan_time_ms: expected scan time in ms (used to calcualted scanning sound phase duration)
 * resp_req: boolean to indicate the type of sounding (TIS or TWAS)
 * address: station address for sounding (15 or less characters) in a null terminated string
 * samples: pointer to a buffer large enough to store 49 symbols (3136 samples)
 * 
 * Returns 1 until complete set of symbols have been generated, then returns 0.
 */
int scanning_sound(int scan_time_ms, int resp_req, char *address, short *samples)
{
	static int ss_time = 0;
	static int encoded_len = 0;
	static int redundant_sound = 0;
	static unsigned int words[5];
	static int type;
	int ret;
	int i;

	/* need to do this initialization only once per sounding */
	if (!encoded_len) {
		type = resp_req ? ALE_WORD_TYPE_TIS : ALE_WORD_TYPE_TWAS;
		ret = build_address_words(type, address, words, &encoded_len);
		if (ret) {
			printf("build_address_words() FAILED (ret = %d)\n", ret);
			return -1;
		}
		// for (i = 0; i < encoded_len; i++){
			// printf("%d = <<%d,%d,%d>>\n",words[i], (words[i]>>0)&0xff,(words[i]>>8)&0xff,(words[i]>>16)&0xff);
		// }
	}

	/* Ta is 392ms, scanning sound time must be n*Ta >= scan_time_ms */
	if (ss_time <= scan_time_ms) {
		i = mod(words, encoded_len, samples);
		if (i >= encoded_len) {
			ss_time += (ALE_TA_MS*encoded_len); // count time
		}
		return 1;
	}

	/* then redundant sound time is 2xTa */
	if (redundant_sound < 2) {
		i = mod(words, encoded_len, samples);
		if (i >= encoded_len) {
			redundant_sound++;
		}
		return 1;		
	}

	/* all done cleanup */
	ss_time = encoded_len = redundant_sound = 0;
	return 0;
}