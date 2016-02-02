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
#define MIN_AVAIL_SIZE 100
#define INCREMENT_SIZE 500

static int define_column(char *buff, const struct column_desc *col);
static int define_primary_key(char *buff, const struct column_desc *col_head);
static int max_pk_idx(const struct column_desc *col_head);

/*
 * This function generates a CREATE TABLE statement from
 * the table struct and the coloumn descriptor list,
 * then writes it to the file specified by `fd'.
 */
void table_desc_to_sql(int fd, const struct table *tbl,
		       const struct column_desc *col_head)
{
	char *buff;
	ssize_t size;
	int stored;
	const struct column_desc *col;

	size = DEF_BUFF_SIZE;
	buff = alloc_buff(size);
	stored = sprintf(buff, "CREATE TABLE %s (\n", tbl->dat_name);
	col = col_head->next;
	while (col) {
		stored += define_column(buff + stored, col);
		col = col->next;
		if (stored + MIN_AVAIL_SIZE > size) {
			size += INCREMENT_SIZE;
			buff = resize_buff(buff, size);
		}
	}

	stored += define_primary_key(buff + stored, col_head);
	strcpy(buff + stored, "\n);\n");

	write_file(fd, buff);
}

/* Writes the primary key list if any, returns the bytes written. */
static int define_primary_key(char *buff, const struct column_desc *col_head)
{
	const struct column_desc *col;
	int pk_max;
	int cnt;
	int i;

	pk_max = max_pk_idx(col_head);
	if (pk_max == 0)
		return 0;

	strcpy(buff, ",\n\tPRIMARY KEY (");
	cnt = strlen(",\n\tPRIMARY KEY (");

	for (i = 1; i <= pk_max; ++i) {
		col = col_head->next;
		while (col->pk_index != i)
			col = col->next;
		if (i < pk_max)
			cnt += sprintf(buff + cnt, "%s, ", col->name);
		else
			cnt += sprintf(buff + cnt, "%s)", col->name);
	}

	return cnt;
}

/* Returns the biggest pk_index among the colunms, or 0 if no pk found */
static int max_pk_idx(const struct column_desc *col_head)
{
	const struct column_desc *col;
	int max;

	max = 0;
	col = col_head->next;
	while (col) {
		max = col->pk_index > max ? col->pk_index : max;
		col = col->next;
	}

	return max;
}

/*
 * This function interprets what a column descriptor struct defines,
 * returns the number of characters populated into the buffer.
 */
static int define_column(char *buff, const struct column_desc *col)
{
	int cnt;

	cnt = 0;
	switch (col->type) {
	case CHAR:
		cnt = sprintf(buff, "\t%s CHAR(%zd)", col->name, col->length);
		break;
	case VARCHAR:
		cnt =
		    sprintf(buff, "\t%s VARCHAR(%zd)", col->name, col->length);
		break;
	case SMALLINT:
		cnt = sprintf(buff, "\t%s SMALLINT", col->name);
		break;
	case INTEGER:
		cnt = sprintf(buff, "\t%s INTEGER", col->name);
		break;
	case DECIMAL:
		cnt = sprintf(buff, "\t%s DECIMAL(%lu, %lu)", col->name,
			      col->length / 100U, col->length % 100U);
		break;
	case TIMESTAMP:
		cnt = sprintf(buff, "\t%s TIMESTAMP", col->name);
		break;
	default:
		err_exit("DataType %d not implemented", col->type);

	}

	if (!col->nullable) {
		strcpy(buff + cnt, " NOT NULL");
		cnt += strlen(" NOT NULL");
	}
	if (col->next) {
		strcpy(buff + cnt, ",\n");
		cnt += strlen(",\n");
	}

	return cnt;
}
