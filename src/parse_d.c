/*
 * parse_d.c - functions for parsing particular field values
 *
 * Copyright 2016 Guo, Xingchun <guoxingchun@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <string.h>

#include "parse_d.h"
#include "util.h"

#define LOW_NIBBLE 0x0F
#define HIGH_NIBBLE 0xF0
#define NEGATIVE_SIGN 0x0D
#define DIGIT_HIGH_NIBBLE 0x30
#define NULL_VAL_INDICATOR 0xFFFF

static char *squeeze_zeros(char *decimal);

/*
 * read a little-endian integer (SMALLINT, INTEGER or BIGINT) from
 * buffer `src', return the corresponding value of type long long
 */
long long parse_ixf_integer(const unsigned char *src, size_t bytes)
{
	long long value;
	size_t i;

	/* SMALLINT: 2 bytes; INTEGER: 4 bytes; BIGINT: 8 bytes */
	assert(bytes == 2 || bytes == 4 || bytes == 8);

	value = 0LL;
	for (i = 0U; i < bytes; ++i)
		value |= (unsigned)(*src++) << (8 * i);

	return value;
}

/*
 * read a floating point number (REAL or DOUBLE) from `src',
 * return the corresponding value of type double
 */
double parse_ixf_float(const unsigned char *src, size_t bytes)
{
	const size_t REAL_SIZE = 4;
	const size_t DOUBLE_SIZE = 8;
	float real;
	double dbl;

	assert(bytes == REAL_SIZE || bytes == DOUBLE_SIZE);
	assert(sizeof(float) == REAL_SIZE && sizeof(double) == DOUBLE_SIZE);

	if (bytes == DOUBLE_SIZE) {
		memcpy(&dbl, src, DOUBLE_SIZE);
		return dbl;
	} else {
		memcpy(&real, src, REAL_SIZE);
		return (double)real;
	}
}

/*
 * Return true if the null indicator represents null, false otherwise.
 * x'0000' for not null, and x'FFFF' for null
 */
bool column_is_null(const unsigned char *null_ind)
{
	return parse_ixf_integer(null_ind, NULL_VAL_IND_BYTES) ==
	    NULL_VAL_INDICATOR;
}

/* Returns the real length of a VARCHAR column value. */
size_t get_varchar_cur_len(const unsigned char *len_ind)
{
	return (size_t) parse_ixf_integer(len_ind, VARCHAR_CUR_LEN_IND_BYTES);
}

/*
 * This function decodes the packed-decimal bytes in `src' into ASCII
 * characters in 'buff', and returns a pointer to the byte following
 * the last character.
 * `data_length' is the original length specified by IXFCLENG
 */
char *decode_packed_decimal(char *buff, const unsigned char *src,
			    size_t data_length)
{
	int precision;
	int scale;
	int bytes;		/* bytes occupied by the packed-decimal */
	const unsigned char *last_byte;	/* of the decimal in `src' */
	bool is_neg;
	char *bp;

	bp = buff;
	precision = (int)data_length / 100;
	scale = (int)data_length % 100;
	assert(precision > 0 && precision < 32);
	assert(scale >= 0 && scale < precision);

	/* extract the sign from the last nibble */
	bytes = (precision + 2) / 2;
	last_byte = src + bytes - 1;
	is_neg = (*last_byte & LOW_NIBBLE) == NEGATIVE_SIGN;
	if (is_neg)
		*bp++ = '-';

	while (src < last_byte) {
		*bp++ = *src >> 4 | DIGIT_HIGH_NIBBLE;
		*bp++ = (*src++ & LOW_NIBBLE) | DIGIT_HIGH_NIBBLE;
	}
	*bp = *src >> 4 | DIGIT_HIGH_NIBBLE;

	/* place the decimal point if necessary */
	if (scale > 0) {
		while (scale-- > 0) {
			*(bp + 1) = *bp;
			bp--;
		}
		*++bp = '.';
	}

	*(bp + scale + 1) = '\0';
	squeeze_zeros(buff);

	return buff + strlen(buff);
}

/* squeeze redundant zeros out of a null-terminated decimal string */
static char *squeeze_zeros(char *decimal)
{
	bool has_sign;
	char *start;

	has_sign = false;
	if (*decimal == '+' || *decimal == '-')
		has_sign = true;

	start = decimal;
	if (has_sign)
		++start;
	while (*start && *start == '0')
		++start;
	if (*start == '\0' || *start == '.')
		--start;
	memmove(decimal + (int)has_sign, start, strlen(start) + 1);

	return decimal;
}
