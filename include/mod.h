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
#ifndef MOD_H
#define MOD_H

/* ALE word types (preambles); first 3 MSBs of ALE word */
#define ALE_WORD_TYPE_THRU	1
#define ALE_WORD_TYPE_TO   	2
#define ALE_WORD_TYPE_CMD  	6
#define ALE_WORD_TYPE_FROM 	4
#define ALE_WORD_TYPE_TIS  	5
#define ALE_WORD_TYPE_TWAS 	3
#define ALE_WORD_TYPE_DATA 	0
#define ALE_WORD_TYPE_REP  	7
#define ALE_WORD_LEN        24
#define ALE_TX_WORD_LEN     49
#define ALE_TA_MS			392

int build_address_words(int type, char *address, unsigned int *words, int *len);
int mod(unsigned int *words, int len, short *samples);
int scanning_sound(int scan_time_ms, int resp_req, char *address, short *samples);

#endif