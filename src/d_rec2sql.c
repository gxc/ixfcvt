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

#include "util.h"
#include "ixfcvt.h"
#include "parse_d.h"

#define IXFDCOLS_OFFSET 8
#define D_REC_BUFF_SIZE 32780

static char *fill_in_arguments(char *buff, const char *table_name,
			       const struct column_desc *col_head);
static char *fill_in_values(char *buff, const unsigned char *d_rec_buff,
			    const struct column_desc *col_head);
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len);

/* convert a D record to a INSERT statement */
void data_record_to_sql(const unsigned char *d_rec_buff,
			const char *table_name,
			const struct column_desc *col_head)
{
	static char buff[D_REC_BUFF_SIZE];
	char *pos;

	pos = fill_in_arguments(buff, table_name, col_head);
	pos = fill_in_values(pos, d_rec_buff, col_head);
	fputs(buff, stdout);
}

/*
 * This function writes arguments of the INSERT statement into the buffer,
 * returns a pointer to the byte following the last written byte.
 * To be precise, it writes
 * "INSERT INTO table_name (column1, column2, ...) VALUES "
 * into the buffer.
 */
static char *fill_in_arguments(char *buff, const char *table_name,
			       const struct column_desc *col_head)
{
	const struct column_desc *col;

	buff += sprintf(buff, "INSERT INTO %s (", table_name);

	col = col_head->next;
	while (col) {
		strcpy(buff, col->name);
		buff += strlen(col->name);
		if (col->next)
			*buff++ = ',';
		col = col->next;
	}
	strcpy(buff, ") VALUES ");
	buff += 9;

	return buff;
}

/*
 * converts a D record to a string of INSERT row values,
 * i.e. "(val1,val2,...);\n" and saves it into `buff'
 */
static char *fill_in_values(char *buff, const unsigned char *d_rec_buff,
			    const struct column_desc *col_head)
{
	const unsigned char *walker;
	struct column_desc *col;
	int data_len;
	long int_val;
	char *ascii_dec;

	col = col_head->next;
	while (col) {
		if (col == col_head->next)
			*buff++ = '(';
		else
			*buff++ = ',';

		walker = d_rec_buff + IXFDCOLS_OFFSET + col->offset;
		if (col->nullable) {
			if (column_is_null(walker)) {
				strcpy(buff, "null");
				buff += 4;
				continue;
			} else {
				/* bypass the null indicator */
				walker += 2;
			}
		}

		switch (col->type) {
		case CHAR:
			/* TO-DO:compose trailing space */
			buff = write_as_sql_str(buff, walker, col->length);
			break;
		case VARCHAR:
			data_len = parse_ixf_integer(walker, 2);
			walker += 2;
			buff = write_as_sql_str(buff, walker, data_len);
			break;
		case SMALLINT:
			int_val = parse_ixf_integer(walker, col->length);
			buff += sprintf(buff, "%hd", (short)int_val);
			break;
		case INTEGER:
			int_val = parse_ixf_integer(walker, col->length);
			buff += sprintf(buff, "%ld", int_val);
			break;
		case DECIMAL:
			/*TO-DO:use alloca ? */
			ascii_dec = decode_packed_decimal(walker, col->length);
			buff += sprintf(buff, ascii_dec);
			free(ascii_dec);
			break;
		case TIMESTAMP:
			buff = write_as_sql_str(buff, walker, col->length);
			break;
		default:
			err_exit("DB2 data type %d not implenmented",
				 col->type);
		}
		col = col->next;
	}
	strcpy(buff, ");\n");
	buff += 3;

	return buff;
}

/*
 * This function escapes single quotes and backslashes in `src',
 * wraps it in single quotes, writes the result into the buffer,
 * and returns a pointer to the byte following the last written
 * byte in the buffer.
 * `len' is the number of characters to be processed in `src'.
 */

/*****BUGS here**********/
/* if src="" len = 0 */
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len)
{
	size_t i;
char a[10000];
if (len > 1000) {
printf("len = %zd\n", len);
memset(a, 0, 10000);
memcpy(a, src, 10000);
puts(a);
}
	
	*buff++ = '\'';
	for (i = 0; i < len; ++i) {
		*buff++ = *src;
		if (*src == '\'' || *src == '\\')
			*buff++ = *src;
		src++;
	}
	*buff++ = '\'';

	return buff;
}

