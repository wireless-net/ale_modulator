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
 * tests for mod functions
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "mod.h"
#include "ale_symbol_library.h"

/*
 * Notes:
 * 
 * ALE Sounding protocol example
 * 
 * Assuming scanning rate of 2 ch / second, and 9 channels -> 4500ms scan time
 * TRW: triple redundant word
 * TWAS: 
 * 
 * 
 *  <-- TRW TWAS Scanning Sound ------------------------------------->     <-- Redundant Sound: 2xTa (complete addr twice) -------------->
 * [TWAS KD6][TWAS KD6][TWAS KD6][DATA DRS][DATA DRS][DATA DRS] x6 ...    [TWAS KD6][TWAS KD6][TWAS KD6] [DATA DRS][DATA DRS][DATA DRS] x2
 * 
 * The scanning sound must be nxTa >= 4500ms = 6xTa! So send [TWAS KD6][TWAS KD6][TWAS KD6][DATA DRS][DATA DRS][DATA DRS] x 6
 * Then send the redundant conclusion section: [TWAS KD6][TWAS KD6][TWAS KD6][DATA DRS][DATA DRS][DATA DRS] x 2
 */

int main()
{
	char *test_address = "KD6DRS";
	char *test_address2 = "KD6DRS012345678";
	char *test_address_odd = "KD6DR";
	char *test_address_bad = "KD6DRS0123456789";
	int ret;
	unsigned int words[5];
	int encoded_len;
	int result_type;
	int i;
	char *type_str[] = {
		"DATA",	//0
		"THRU",	//1
		"TO"  ,	//2
		"TWAS",	//3
		"FROM",	//4
		"TIS" ,	//5
		"CMD" ,	//6
		"REP" ,	//7
	};
	char c1,c2,c3;
	short mod_samples[ALE_SYMBOL_SIZE * ALE_TX_WORD_LEN];
	int fd;

	/////////////////////////////////////////////////////
	// TEST THE ADDRESS WORD ENCODER
	/////////////////////////////////////////////////////

	printf("TEST build_address_words...\n");
	ret = build_address_words(ALE_WORD_TYPE_TWAS, test_address, words, &encoded_len);
	if (ret) {
		printf("FAILED (ret = %d)\n", ret);
	}
	if (encoded_len != 2) {
		printf("FAILED (encoded_len = %d, expecting 2)\n", encoded_len);
	}
	// expect encoded word 0: TWAS KD6; 3 4b 44 36
	// expect encoded word 1: DATA DRS; 0 44 52 83 
	for (i = 0; i < encoded_len; i++) {
		result_type = (words[i] >> 21) & 0x07;
		c1			= (words[i] >> 14) & 0x7f; 
		c2			= (words[i] >> 7) & 0x7f; 
		c3			= (words[i] >> 0) & 0x7f;
		printf("%s %c %c %c\n", type_str[result_type], c1, c2, c3); 
	}
	printf("encoded_len = %d, 0x%x 0x%x\n", encoded_len, words[0], words[1]);
	printf("PASS\n");

	// test LONG address
	printf("TEST LONG build_address_words...\n");
	ret = build_address_words(ALE_WORD_TYPE_TWAS, test_address2, words, &encoded_len);
	if (ret) {
		printf("FAILED (ret = %d)\n", ret);
	}
	if (encoded_len != 5) {
		printf("FAILED (encoded_len = %d, expecting 5)\n", encoded_len);
	}
	for (i = 0; i < encoded_len; i++) {
		result_type = (words[i] >> 21) & 0x07;
		c1			= (words[i] >> 14) & 0x7f; 
		c2			= (words[i] >> 7) & 0x7f; 
		c3			= (words[i] >> 0) & 0x7f;
		printf("%s %c %c %c\n", type_str[result_type], c1, c2, c3); 
	}
	printf("PASS\n");

	// test odd address
	printf("TEST ODD build_address_words...\n");
	ret = build_address_words(ALE_WORD_TYPE_TWAS, test_address_odd, words, &encoded_len);
	if (ret) {
		printf("FAILED (ret = %d)\n", ret);
	}
	if (encoded_len != 2) {
		printf("FAILED (encoded_len = %d, expecting 2)\n", encoded_len);
	}
	for (i = 0; i < encoded_len; i++) {
		result_type = (words[i] >> 21) & 0x07;
		c1			= (words[i] >> 14) & 0x7f; 
		c2			= (words[i] >> 7) & 0x7f; 
		c3			= (words[i] >> 0) & 0x7f;
		printf("%s %c %c %c\n", type_str[result_type], c1, c2, c3); 
	}
	printf("PASS\n");

	// test invalid length
	ret = build_address_words(ALE_WORD_TYPE_TWAS, test_address_bad, words, &encoded_len);
	if (ret) {
		printf("PASS (ret = %d)\n", ret);
	} else {
		printf("FAILED to reject bad address\n");
	}

	//////////////////////////////////////////////////////////
	// Test the modulator
	//////////////////////////////////////////////////////////
	ret = build_address_words(ALE_WORD_TYPE_TWAS, test_address, words, &encoded_len);
	if (ret) {
		printf("build_address_words() FAILED (ret = %d)\n", ret);
		return -1;
	}

	fd = open("mod.raw", O_WRONLY | O_CREAT, 0664);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	i = mod(words, encoded_len, mod_samples);
	while (i < encoded_len) {
		ret = write(fd, mod_samples, ALE_SYMBOL_SIZE*ALE_TX_WORD_LEN*sizeof(short));
		if (ret < ALE_SYMBOL_SIZE*ALE_TX_WORD_LEN*sizeof(short)) {
			perror("write");
			return 0;
		}
		i = mod(words, encoded_len, mod_samples);
	}
	close(fd);

	printf("BEGIN SOUNDING\n");
	///////////////////////////////////////////////////////
	// Now test sounding
	///////////////////////////////////////////////////////
	fd = open("sound.raw", O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	while (scanning_sound(4200, 0, test_address, mod_samples)) {
		ret = write(fd, mod_samples, ALE_SYMBOL_SIZE*ALE_TX_WORD_LEN*sizeof(short));
		if (ret < ALE_SYMBOL_SIZE*ALE_TX_WORD_LEN*sizeof(short)) {
			perror("write");
			return 0;
		}		
	}
	close(fd);

	return 0;
}