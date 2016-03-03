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

#include "ixfcvt.h"
#include "util.h"

#define DEF_BUFF_SIZE 1000
#define MIN_AVAIL_SIZE 200
#define INCREMENT_SIZE 500

static int sprint_column(char *buff, const struct column_desc *col);
static int sprint_anon_pk(char *buff, const struct column_desc *col_head);
static void sprint_named_pk(char *buff, const struct table_desc *tbl);
static int max_pk_pos(const struct column_desc *col_head);
static void *ensure_capacity(void *buff, size_t * cur_size, size_t used);

/*
 * This function generates a CREATE TABLE statement from
 * `tbl' and writes it to the file specified by `fd'.
 */
void table_desc_to_sql(int fd, const struct table_desc *tbl)
{
	char *buff;
	size_t size;
	size_t stored;
	const struct column_desc *col;

	size = DEF_BUFF_SIZE;
	buff = alloc_buff(size);
	stored = sprintf(buff, "CREATE TABLE %s (\n", tbl->t_name);
	for (col = tbl->c_head; col; col = col->next) {
		buff = ensure_capacity(buff, &size, stored);
		stored += sprint_column(buff + stored, col);
	}

	buff = ensure_capacity(buff, &size, stored);
	if (tbl->t_pkname && strlen(tbl->t_pkname)) {
		strcpy(buff + stored, "\n);\n\n");
		stored += strlen("\n);\n\n");
		sprint_named_pk(buff + stored, tbl);
	} else {
		stored += sprint_anon_pk(buff + stored, tbl->c_head);
		strcpy(buff + stored, "\n);\n");
	}

	write_file(fd, buff);
	free_buff(buff);
}

/* enlarge buffer if necessary */
static void *ensure_capacity(void *buff, size_t * cur_size, size_t used)
{
	if (used + MIN_AVAIL_SIZE > *cur_size) {
		buff = resize_buff(buff, *cur_size + INCREMENT_SIZE);
		*cur_size += INCREMENT_SIZE;
	}

	return buff;
}

/* Writes the named primary key list to `buff' */
static void sprint_named_pk(char *buff, const struct table_desc *tbl)
{
	char *const FMT = "ALTER TABLE %s ADD CONSTRAINT %s PRIMARY KEY (";
	const struct column_desc *col;
	int pk_max;
	int cnt;
	int i;

	pk_max = max_pk_pos(tbl->c_head);
	if (pk_max == 0)
		return;

	cnt = sprintf(buff, FMT, tbl->t_name, tbl->t_pkname);

	for (i = 1; i <= pk_max; ++i) {
		for (col = tbl->c_head; col; col = col->next)
			if (col->c_pkpos == i)
				break;
		if (i < pk_max)
			cnt += sprintf(buff + cnt, "%s, ", col->c_name);
		else
			cnt += sprintf(buff + cnt, "%s);\n", col->c_name);
	}
}

/* Writes the anonymous primary key list to `buff', returns the bytes written. */
static int sprint_anon_pk(char *buff, const struct column_desc *col_head)
{
	const struct column_desc *col;
	int pk_max;
	int cnt;
	int i;

	pk_max = max_pk_pos(col_head);
	if (pk_max == 0)
		return 0;

	strcpy(buff, ",\n\tPRIMARY KEY (");
	cnt = strlen(",\n\tPRIMARY KEY (");

	for (i = 1; i <= pk_max; ++i) {
		for (col = col_head; col; col = col->next)
			if (col->c_pkpos == i)
				break;
		if (i < pk_max)
			cnt += sprintf(buff + cnt, "%s, ", col->c_name);
		else
			cnt += sprintf(buff + cnt, "%s)", col->c_name);
	}

	return cnt;
}

/* Returns the biggest c_pkpos among the colunms, or 0 if no pk found */
static int max_pk_pos(const struct column_desc *col_head)
{
	const struct column_desc *col;
	int max;

	max = 0;
	for (col = col_head; col; col = col->next)
		if (col->c_pkpos > max)
			max = col->c_pkpos;

	return max;
}

/*
 * This function interprets and writes the definition of `col',
 * returns the number of characters populated into the buffer.
 */
static int sprint_column(char *buff, const struct column_desc *col)
{
	int cnt;

	cnt = 0;
	switch (col->c_type) {
	case CHAR:
		cnt = sprintf(buff, "\t%s CHAR(%zd)", col->c_name, col->c_len);
		break;
	case VARCHAR:
		cnt =
		    sprintf(buff, "\t%s VARCHAR(%zd)", col->c_name, col->c_len);
		break;
	case SMALLINT:
		cnt = sprintf(buff, "\t%s SMALLINT", col->c_name);
		break;
	case INTEGER:
		cnt = sprintf(buff, "\t%s INTEGER", col->c_name);
		break;
	case BIGINT:
		cnt = sprintf(buff, "\t%s BIGINT", col->c_name);
		break;
	case DECIMAL:
		cnt = sprintf(buff, "\t%s DECIMAL(%lu, %lu)", col->c_name,
			      col->c_len / 100U, col->c_len % 100U);
		break;
	case DATE:
		cnt = sprintf(buff, "\t%s DATE", col->c_name);
		break;
	case TIME:
		cnt = sprintf(buff, "\t%s TIME", col->c_name);
		break;
	case TIMESTAMP:
		cnt = sprintf(buff, "\t%s TIMESTAMP", col->c_name);
		break;
	default:
		fmt_err_exit("DataType %d not implemented", col->c_type);

	}

	if (!col->c_nullable) {
		strcpy(buff + cnt, " NOT NULL");
		cnt += strlen(" NOT NULL");
	}
	if (col->next) {
		strcpy(buff + cnt, ",\n");
		cnt += strlen(",\n");
	}

	return cnt;
}
