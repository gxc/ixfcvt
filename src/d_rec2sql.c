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
#include <string.h>

#include "util.h"
#include "ixfcvt.h"
#include "parse_d.h"

#define D_REC_BUFF_SIZE 32780

static char *fill_in_arguments(char *buff, const char *table_name,
			       const struct column_desc *col_head);
static char *fill_in_values(char *buff, const unsigned char *d_rec_buff,
			    struct column_desc **pcol);
static char *fill_in_a_value(char *buff, const unsigned char *src,
			     const struct column_desc *col);
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len);

/* convert D records of a row to a INSERT statement */
void data_record_to_sql(int fd,
			const unsigned char *d_rec_buff,
			const struct table *tbl,
			const struct column_desc *col_head)
{
	static char buff[D_REC_BUFF_SIZE];
	static char *pos = buff;
	static struct column_desc *next_col = NULL;

	/* next_col == col_head means a new row */
	if (!next_col)
		next_col = (struct column_desc *)col_head;
	if (next_col == col_head)
		pos = fill_in_arguments(buff, tbl->dat_name, col_head);
	pos = fill_in_values(pos, d_rec_buff, &next_col);
	if (!next_col) {
		next_col = (struct column_desc *)col_head;
		pos = buff;
		write_file(fd, buff);
	}
}

/*
 * This function writes arguments of the INSERT statement into
 * the buffer, returns a pointer to the byte following the last
 * written byte. To be precise, it writes
 * "INSERT INTO table_name (column1, column2, ...) VALUES ".
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
	buff += strlen(") VALUES ");

	return buff;
}

/*
 * Converts a D record to a string of row values, i.e.
 * "(val1,val2,...);\n" or a part of them, and saves it into `buff'.
 * If a row consists of mutiple D records, `*pcol' returns the
 * column corresponding to the beginning of the next D record,
 * or NULL to indicate the end of a row.
 */
static char *fill_in_values(char *buff, const unsigned char *d_rec_buff,
			    struct column_desc **pcol)
{
	const unsigned char *walker;
	struct column_desc *col;

	col = *pcol;
	do {
		/* head of the struct means a new row */
		if (col->name == NULL) {
			col = col->next;
			*buff++ = '(';
		} else {
			*buff++ = ',';
		}

		walker = d_rec_buff + IXFDCOLS_OFFSET + col->offset;
		buff = fill_in_a_value(buff, walker, col);
		col = col->next;
	} while (col && col->offset);

	/* If col points to NULL, all values in a
	   row processed. Or if col->offset is 0,
	   values are stored in the next D record */
	*pcol = col;
	if (!col) {
		strcpy(buff, ");\n");
		buff += strlen(");\n");
	}

	return buff;
}

/*
 * writes the string value of the specified column in
 * a row from `src' to `buff'
 */
static char *fill_in_a_value(char *buff, const unsigned char *src,
			     const struct column_desc *col)
{
	unsigned cur_len;
	long num_val;

	if (col->nullable) {
		if (column_is_null(src)) {
			strcpy(buff, "null");
			buff += strlen("null");
			return buff;
		} else {
			/* bypass the null indicator */
			src += NULL_VAL_IND_BYTES;
		}
	}

	switch (col->type) {
	case CHAR:
		buff = write_as_sql_str(buff, src, col->length);
		break;
	case VARCHAR:
		cur_len = get_varchar_cur_len(src);
		src += VARCHAR_CUR_LEN_IND_BYTES;
		buff = write_as_sql_str(buff, src, cur_len);
		break;
	case SMALLINT:
		num_val = parse_ixf_integer(src, col->length);
		buff += sprintf(buff, "%hd", (short)num_val);
		break;
	case INTEGER:
		num_val = parse_ixf_integer(src, col->length);
		buff += sprintf(buff, "%ld", num_val);
		break;
	case DECIMAL:
		buff = decode_packed_decimal(buff, src, col->length);
		break;
	case TIMESTAMP:
		buff = write_as_sql_str(buff, src, col->length);
		break;
	default:
		err_exit("DB2 data type %d not implenmented", col->type);
	}

	return buff;
}

/*
 * This function escapes single quotes and backslashes in `src',
 * wraps it in single quotes, writes the result into the buffer,
 * and returns a pointer to the byte following the last written
 * byte in the buffer.
 * `len' is the number of characters to be processed in `src'.
 */
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len)
{
	size_t i;

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
