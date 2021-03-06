/*
 * d2sql.c - output a D record
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

#include <float.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ixfcvt.h"
#include "parse_d.h"
#include "util.h"

static void init_static_args(const struct summary *sum,
			     const struct table_desc *tbl);
static void dispose_static_buffs(void);
static size_t insert_into_clause_size(const struct table_desc *tbl);
static void gen_insert_into_clause(char *buff, const struct table_desc *tbl);
static size_t max_d_values_size(const struct table_desc *tbl);
static size_t col_value_size(const struct column_desc *col);
static void fill_in_values(char *buff, const unsigned char *rec,
			   const struct column_desc *col_head,
			   struct column_desc **colptr);
static char *fill_in_a_value(char *buff, const unsigned char *src,
			     const struct column_desc *col);
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len);

static char *restrict insert_into_clause;
static char *restrict values_buff;
static size_t values_buff_size;
static bool escape_backslash;

/* convert a D record to (part of) an INSERT statement */
void d_record_to_sql(int ofd, const unsigned char *rec,
		     const struct summary *sum, const struct table_desc *tbl)
{
	static struct column_desc *col = NULL;
	static long recs = 0L;
	static long rows = 0L;

	if (recs == 0L)
		init_static_args(sum, tbl);

	/* a new row */
	if (!col) {
		col = tbl->c_head;
		write_file(ofd, insert_into_clause);
		++rows;
	}

	/* output values of a D record */
	memset(values_buff, 0x00, values_buff_size);
	fill_in_values(values_buff, rec, tbl->c_head, &col);
	write_file(ofd, values_buff);
	++recs;

	if (ofd != STDOUT_FILENO)
		show_progress(recs, sum->s_dcnt);

	/* output a COMMIT statement if necessary */
	if (!col && sum->s_cmtsz != 0
	    && (rows == sum->s_cmtsz || recs == sum->s_dcnt)) {
		write_file(ofd, "commit;\n");
		rows = 0;
	}

	if (recs == sum->s_dcnt)
		dispose_static_buffs();
}

/*
 * initialize file scope static variables
 * read out whether to escape backslashes, allocate memory
 * for buffers, build an INSERT INTO clause
 */
static void init_static_args(const struct summary *sum,
			     const struct table_desc *tbl)
{
	size_t size;

	escape_backslash = sum->s_escbs;

	size = insert_into_clause_size(tbl);
	insert_into_clause = alloc_buff(size);
	gen_insert_into_clause(insert_into_clause, tbl);

	size = max_d_values_size(tbl);
	values_buff = alloc_buff(size);
	values_buff_size = size;
}

/* free buffers `insert_into_clause' and `values_buff' */
static void dispose_static_buffs(void)
{
	free_buff(insert_into_clause);
	free_buff(values_buff);
}

/* calculate and return the required size of buffer `insert_into_clause' */
static size_t insert_into_clause_size(const struct table_desc *tbl)
{
	const size_t COMMA_LEN = 1;
	const size_t NULL_TERM_LEN = 1;
	size_t size;
	const struct column_desc *col;

	size = strlen("INSERT INTO ");
	size += strlen(tbl->t_name);
	size += strlen(" (");
	for (col = tbl->c_head; col; col = col->next)
		size += strlen(col->c_name) + COMMA_LEN;
	size += strlen(" VALUES ");
	size += NULL_TERM_LEN;

	return size;
}

/*
 * Build an INSERT INTO clause, fills it in `buff', that is,
 * "INSERT INTO table_name (column1, column2, ...) VALUES ".
 */
static void gen_insert_into_clause(char *buff, const struct table_desc *tbl)
{
	const struct column_desc *col;

	buff += sprintf(buff, "INSERT INTO %s (", tbl->t_name);

	for (col = tbl->c_head; col; col = col->next) {
		strcpy(buff, col->c_name);
		buff += strlen(col->c_name);
		*buff++ = ',';
	}

	/* eat last comma */
	strcpy(--buff, ") VALUES ");
}

/* calculate and return max size of a string representation of a D record */
static size_t max_d_values_size(const struct table_desc *tbl)
{
	const size_t COMMA_LEN = 1;
	const size_t WRAPPER_LEN = 4;
	size_t max;
	size_t size;
	const struct column_desc *col;

	max = 0;
	size = 0;
	for (col = tbl->c_head; col; col = col->next) {
		size += col_value_size(col) + COMMA_LEN;
		if (!col->next || col->next->c_offset == 0) {
			max = size > max ? size : max;
			size = 0;
		}
	}

	return max + WRAPPER_LEN;
}

/* return size of the string representation of column pointed to by `col' */
static size_t col_value_size(const struct column_desc *col)
{
	const size_t SIGN_LEN = 1;
	const size_t SINGLE_QUOTES_LEN = 2;
	const size_t SMALLINT_STR_LEN = 6;	/* -32768 */
	const size_t INTEGER_STR_LEN = 11;	/* -2147483648 */
	const size_t BIGINT_STR_LEN = 20;	/* -9223372036854775808 */
	const size_t REAL_STR_LEN = FLT_DIG + 6;	/* -d.d{FLT_DIG-1}E+dd */
	const size_t DOUBLE_STR_LEN = DBL_DIG + 7;	/* -d.d{DBL_DIG-1}E+ddd */

	size_t size;

	size = 0;
	switch (col->c_type) {
	case CHAR:
	case VARCHAR:
	case DATE:
	case TIME:
	case TIMESTAMP:
		size = col->c_len + SINGLE_QUOTES_LEN;
		break;
	case SMALLINT:
		size = SMALLINT_STR_LEN;
		break;
	case INTEGER:
		size = INTEGER_STR_LEN;
		break;
	case BIGINT:
		size = BIGINT_STR_LEN;
		break;
	case DECIMAL:
		size = col->c_len / 100 + SIGN_LEN;
		break;
	case FLOATING_POINT:
		size = col->c_len == 4 ? REAL_STR_LEN : DOUBLE_STR_LEN;
		break;
	default:
		fmt_err_exit(E_DATA_TYPE_NOT_IMPL, col->c_type);
	}

	return size;
}

/*
 * Converts a D record to a string of row values, i.e.
 * "(val1,val2,...);\n", or part of it, saves it into `buff', and
 * returns a pointer to the byte following the last written byte.
 * If a row consists of mutiple D records, `*colptr' returns the
 * column corresponding to the beginning of the next D record,
 * or NULL to indicate the end of a row.
 */
static void fill_in_values(char *buff, const unsigned char *rec,
			   const struct column_desc *col_head,
			   struct column_desc **colptr)
{
	const unsigned char *pos;
	struct column_desc *col;

	col = *colptr;
	do {
		if (col == col_head)
			*buff++ = '(';
		else
			*buff++ = ',';

		pos = rec + IXFDCOLS_OFFSET + col->c_offset;
		buff = fill_in_a_value(buff, pos, col);
		col = col->next;
	} while (col && col->c_offset > 0);
	/* col->c_offset == 0 means a new record beginning */

	if (!col)		/* end of a row */
		strcpy(buff, ");\n");

	*colptr = col;
}

/*
 * write the string value of the column pointed to by `col' from `src'
 * to `buff'
 */
static char *fill_in_a_value(char *buff, const unsigned char *src,
			     const struct column_desc *col)
{
	size_t cur_len;		/*current length of variable-length string */
	long long num_val;	/* integer */
	double flt_val;		/* floating point number */

	if (col->c_nullable) {
		if (column_is_null(src)) {
			strcpy(buff, "null");
			buff += strlen("null");
			return buff;
		} else {
			/* bypass the null indicator */
			src += NULL_VAL_IND_BYTES;
		}
	}

	switch (col->c_type) {
	case CHAR:
	case DATE:
	case TIME:
	case TIMESTAMP:
		buff = write_as_sql_str(buff, src, col->c_len);
		break;
	case VARCHAR:
		cur_len = get_varchar_cur_len(src);
		src += VARCHAR_CUR_LEN_IND_BYTES;
		buff = write_as_sql_str(buff, src, cur_len);
		break;
	case SMALLINT:
		num_val = parse_ixf_integer(src, col->c_len);
		buff += sprintf(buff, "%hd", (short)num_val);
		break;
	case INTEGER:
		num_val = parse_ixf_integer(src, col->c_len);
		buff += sprintf(buff, "%ld", (long)num_val);
		break;
	case BIGINT:
		num_val = parse_ixf_integer(src, col->c_len);
		buff += sprintf(buff, "%lld", num_val);
		break;
	case DECIMAL:
		buff = decode_packed_decimal(buff, src, col->c_len);
		break;
	case FLOATING_POINT:
		flt_val = parse_ixf_float(src, col->c_len);
		buff += sprintf(buff, "%.*G",
				col->c_len == 4 ? FLT_DIG : DBL_DIG, flt_val);
		break;
	default:
		fmt_err_exit(E_DATA_TYPE_NOT_IMPL, col->c_type);
	}

	return buff;
}

/*
 * This function escapes single quotes and backslashes (if required),
 * wraps it in single quotes, writes the result into the buffer,
 * and returns a pointer to the byte following the last written
 * byte in the buffer.
 * `len' is the number of characters to be processed in `src'.
 */
static char *write_as_sql_str(char *buff, const unsigned char *src, size_t len)
{
	size_t i;
	const char *orig;

	orig = (const char *)src;
	*buff++ = '\'';
	for (i = 0; i < len; ++i) {
		*buff++ = *orig;
		if (*orig == '\'' || (*orig == '\\' && escape_backslash))
			*buff++ = *orig;
		orig++;
	}
	*buff++ = '\'';

	return buff;
}
