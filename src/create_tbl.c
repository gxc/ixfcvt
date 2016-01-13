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

/* char buff[2000]; */
static int desc_col(char *buff, const struct column_descriptor *col);

char *get_create_table_ddl(char *buff, const struct table *tbl, const struct column_descriptor *col_head)
{
	const struct column_descriptor *col;
	int num_stored;

	num_stored = sprintf(buff, "CREATE TABLE %s (\n", tbl->dat_name);
	col = col_head->next;
	while (col)
	{
		num_stored += desc_col(buff + num_stored, col);
		col = col->next;
	}

	num_stored -= 2; /* eat the last comma and newline */
	strcpy(buff + num_stored, "\n);\n");

	return buff;
}

static int desc_col(char *buff, const struct column_descriptor *col)
{
	int cnt;

	switch (col->type) {
	case CHAR:
		cnt = sprintf(buff, "\t%s CHAR(%zd)", col->name, col->length);
		break;
	case VARCHAR:
		cnt = sprintf(buff, "\t%s VARCHAR(%zd)", col->name, col->length);
		break;
	case SMALLINT:
		cnt = sprintf(buff, "\t%s SMALLINT", col->name);
		break;
	case INTEGER:
		cnt = sprintf(buff, "\t%s INTEGER", col->name);
		break;
	case DECIMAL:
		cnt = sprintf(buff, "\t%s DECIMAL(%lu, %lu)", col->name, col->length / 100U, col->length % 100U);
		break;
	case TIMESTAMP:
		cnt = sprintf(buff, "\t%s TIMESTAMP", col->name);
		break;
	default:
		err_exit("DataType %d not implemented", col->type);

	}

	if (col->nullable) {
		strcpy(buff + cnt, ",\n");
		cnt += 2;
	} else {
		strcpy(buff + cnt, " NOT NULL,\n");
		cnt += 11;
	}

	return cnt;
}

