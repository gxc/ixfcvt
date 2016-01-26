/*
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

#include <string.h>
#include <assert.h>

#include "util.h"
#include "parse_d.h"

#define LOW_NIBBLE 0x0F
#define HIGH_NIBBLE 0xF0
#define NEGATIVE_SIGN 0x0D
#define DIGIT_HIGH_NIBBLE 0x30
#define NULL_VAL_INDICATOR 0xFFFF

static char *squeeze_zeros(char *decimal);

/*
 * read 2-byte or 4-byte little-endian integer from buffer `src'
 * return the corresponding numeric value of type
 */
long parse_ixf_integer(const unsigned char *src, size_t bytes)
{
	long value;
	size_t i;

	/* SMALLINT: 2 bytes; INTEGER: 4bytes */
	assert(bytes == 2 || bytes == 4);
	value = 0;
	for (i = 0; i < bytes; ++i)
		value |= (unsigned)(*src++) << (8 * i);

	return value;
}

/*
 * Return true if the null indicator represents null, false otherwise.
 * x'0000' for not null, and x'FFFF' for null
 */
_Bool column_is_null(const unsigned char *null_ind)
{
	long ind;

	ind = parse_ixf_integer(null_ind, NULL_VAL_IND_BYTES);
	return ind == NULL_VAL_INDICATOR;
}

/* Returns the real length of a VARCHAR column value. */
size_t get_varchar_cur_len(const unsigned char *len_ind)
{
	return parse_ixf_integer(len_ind, VARCHAR_CUR_LEN_IND_BYTES);
}

/*
 * This function decodes the packed-decimal bytes into a newly
 * allocated ASCII string, and returns a pointer to the new string.
 * `data_length' is the original length specified by IXFCLENG
 */
char *decode_packed_decimal(const unsigned char *buff, size_t data_length)
{
	int precision;
	int scale;
	size_t bytes;
	_Bool is_neg;
	char *ascii_buf;
	size_t ascii_buf_size;
	char *asc_walker;
	const unsigned char *last_byte;

	precision = data_length / 100;
	scale = data_length % 100;
	bytes = (precision + 2) / 2;
	assert(precision > 0 && precision < 32);
	assert(scale >= 0 && scale < precision);

	/* allocate 2 extra bytes for a decimal point and a trailing '\0' */
	ascii_buf_size = bytes * 2 + 2;
	ascii_buf = alloc_buff(ascii_buf_size);
	memset(ascii_buf, 0x00, ascii_buf_size);
	asc_walker = ascii_buf;

	/* extract the sign from the last nibble */
	last_byte = buff + bytes - 1;
	is_neg = (*last_byte & LOW_NIBBLE) == NEGATIVE_SIGN;
	if (is_neg)
		*asc_walker++ = '-';

	while (buff < last_byte) {
		*asc_walker++ = *buff >> 4 | DIGIT_HIGH_NIBBLE;
		*asc_walker++ = (*buff++ & LOW_NIBBLE) | DIGIT_HIGH_NIBBLE;
	}
	*asc_walker = *buff >> 4 | DIGIT_HIGH_NIBBLE;

	/* place the decimal point if necessary */
	if (scale) {
		while (scale-- > 0) {
			*(asc_walker + 1) = *asc_walker;
			asc_walker--;
		}
		*++asc_walker = '.';
	}

	return squeeze_zeros(ascii_buf);
}

/* squeeze redundant zeros out of a null-terminated decimal string */
static char *squeeze_zeros(char *decimal)
{
	_Bool has_sign;
	char *start;

	has_sign = 0;
	if (*decimal == '+' || *decimal == '-')
		has_sign = 1;

	start = decimal + has_sign;
	while (*start && *start == '0')
		++start;
	if (!*start || *start == '.')
		--start;
	memmove(decimal + has_sign, start, strlen(start) + 1);

	return decimal;
}
