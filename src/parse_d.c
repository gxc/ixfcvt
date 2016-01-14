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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "ixfcvt.h"

#define IXFDCOLS_OFFSET 8
#define D_REC_BUFF_SIZE 32780
#define LOW_NIBBLE 0x0F
#define HIGH_NIBBLE 0xF0
#define NEGATIVE_SIGN 0x0D
#define DIGIT_HIGH_NIBBLE 0x30

static long parse_ixf_integer(const unsigned char *src, size_t bytes);
static _Bool column_is_null(const unsigned char *null_ind);
static char *squeeze_zeros(char *decimal);
static char *decode_packed_decimal(const unsigned char *buff,
				   size_t data_length);

void parse_data_record(const unsigned char *d_rec_buff,
		       const struct column_desc *col_desc_head)
{
	static unsigned char buff[D_REC_BUFF_SIZE];
	const unsigned char *walker;	/* walk through d_rec_buff */
	struct column_desc *col_desc;
	size_t data_len;
	char *ascii_dec;

	col_desc = col_desc_head->next;
	memset(buff, 0x00, D_REC_BUFF_SIZE);

	puts("======== DATA RECORD BEGIN ========");
	while (col_desc) {
		walker = d_rec_buff + IXFDCOLS_OFFSET + col_desc->offset;
		if (col_desc->nullable) {
			if (column_is_null(walker)) {
				printf("null");
				continue;
			} else {	/* bypass the null indicator */
				walker += 2;
			}
		}

		switch (col_desc->type) {
		case CHAR:
			memcpy(buff, walker, col_desc->length);
			printf("CHAR(%zd): %.*s ", col_desc->length,
			       (int)col_desc->length, buff);
			break;
		case VARCHAR:
			data_len = (size_t) parse_ixf_integer(walker, 2);
			memcpy(buff, walker + 2, data_len);
			printf("VARCHAR(%zd of %zd): %.*s ", data_len,
			       col_desc->length, (int)data_len, buff);
			break;
		case SMALLINT:
			printf("SMALLINT: %hd ",
			       (short)parse_ixf_integer(walker,
							col_desc->length));
			break;
		case INTEGER:
			printf("INTEGER: %ld ",
			       parse_ixf_integer(walker, col_desc->length));
			break;
		case DECIMAL:
			ascii_dec =
			    decode_packed_decimal(walker, col_desc->length);
			printf("DECIMAL: %s ", ascii_dec);
			free(ascii_dec);
			break;
		case TIMESTAMP:
			memcpy(buff, walker, col_desc->length);
			printf("TIMESTAMP: %.*s ", (int)col_desc->length, buff);
			break;
		default:
			err_exit
			    ("DATA RECORD: DB2 data type %d not implenmented",
			     col_desc->type);
		}
		col_desc = col_desc->next;
	}

	puts("\n========= DATA RECORD END =========");
}

/*
 * read 2-byte or 4-byte little-endian integer from buffer `src'
 * return the corresponding numeric value of type
 */
static long parse_ixf_integer(const unsigned char *src, size_t bytes)
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
static _Bool column_is_null(const unsigned char *null_ind)
{
	return (unsigned short)parse_ixf_integer(null_ind, 2) == 0xFFFF;
}

/*
 * This function decodes the packed-decimal bytes into a newly
 * allocated ASCII string, and returns a pointer to the new string.
 * `data_length' is the original length specified by IXFCLENG
 */
static char *decode_packed_decimal(const unsigned char *buff,
				   size_t data_length)
{
	int precision;
	int scale;
	int bytes;
	_Bool is_neg;
	char *ascii_buf;
	char *asc_walker;
	const unsigned char *last_byte;

	precision = data_length / 100;
	scale = data_length % 100;
	bytes = (precision + 2) / 2;
	assert(precision > 0 && precision < 32);
	assert(scale >= 0 && scale < precision);

	/* allocate 2 extra bytes for a decimal point and a trailing '\0' */
	ascii_buf = calloc(1, bytes * 2 + 2);
	if (!ascii_buf)
		err_exit("not enough memory available");
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
