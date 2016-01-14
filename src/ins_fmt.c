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

static int format_column(char *pos, const struct column_desc *col);

/* format a INSERT template to be used by printf family functions */
char *format_insert(char *buff, const char *table_name,
		    const struct column_desc *col_head)
{
	char *cur;
	const struct column_desc *col;

	cur = buff;
	cur += sprintf(cur, "INSERT INTO %s (", table_name);

	col = col_head->next;
	while (col) {
		strcpy(cur, (char *)col->name);
		cur += strlen((char *)col->name);
		if (col->next)
			*cur++ = ',';
		col = col->next;
	}
	strcpy(cur, ") VALUES (");
	cur += 10;

	col = col_head->next;
	while (col) {
		cur += format_column(cur, col);
		col = col->next;
	}

	strcpy(cur, ");\n");

	return buff;
}

/*
 * puts a specifier for `col' into the particular position of
 * the buffer, returns the number of characters populated
 */
static int format_column(char *pos, const struct column_desc *col)
{
	int cnt;

	cnt = 0;
	switch (col->type) {
	case CHAR:
	case VARCHAR:
	case DECIMAL:
	case TIMESTAMP:
		strcpy(pos, "%s");
		cnt += 2;
		break;
	case SMALLINT:
		strcpy(pos, "%hd");
		cnt += 3;
		break;
	case INTEGER:
		strcpy(pos, "%ld");
		cnt += 3;
		break;
	default:
		err_exit("DataType %d not implemented", col->type);
	}

	if (col->next)
		*(pos + cnt++) = ',';

	return cnt;
}
