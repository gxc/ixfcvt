/*
 * parse_c.c - parse a C record
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

#include <string.h>

#include "ixfcvt.h"
#include "util.h"

#define IXFCNAML_OFFSET 1
#define IXFCNAML_BYTES 3
#define IXFCNAME_OFFSET 4
#define IXFCKPOS_OFFSET 263
#define IXFCKPOS_BYTES 2
#define IXFCTYPE_OFFSET 266
#define IXFCTYPE_BYTES 3
#define IXFCLENG_OFFSET 279
#define IXFCLENG_BYTES 5
#define IXFCPOSN_OFFSET 287
#define IXFCPOSN_BYTES 6
#define IXFCNULL_OFFSET 260
#define COL_ATTR_BUFF_SIZE 7	/* max of IXFCxxxx_BYTES + 1 */

static void tweak_col_length(struct column_desc *col);
static int get_pk_pos(const char *pkpos);

/* ignore IXFCDEFL, IXFCDEFV now */
/* parse a C record, store the info in a column_desc struct */
void parse_c_record(const unsigned char *rec, struct column_desc *col)
{
	char buff[COL_ATTR_BUFF_SIZE];
	int c_name_len;

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFCNAML_OFFSET, IXFCNAML_BYTES);
	c_name_len = str_to_long(buff);
	col->c_name = alloc_buff(c_name_len + 1);
	memcpy(col->c_name, rec + IXFCNAME_OFFSET, c_name_len);
	col->c_name[c_name_len] = '\0';

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFCKPOS_OFFSET, IXFCKPOS_BYTES);
	col->c_pkpos = get_pk_pos(buff);

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFCTYPE_OFFSET, IXFCTYPE_BYTES);
	col->c_type = str_to_long(buff);

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFCLENG_OFFSET, IXFCLENG_BYTES);
	col->c_len = str_to_long(buff);
	tweak_col_length(col);

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFCPOSN_OFFSET, IXFCPOSN_BYTES);
	/* the IXFDCOLS field of the D record starts at 1 (not 0) */
	col->c_offset = str_to_long(buff) - 1;

	col->c_nullable = rec[IXFCNULL_OFFSET] == 'Y';
}

/*
 * Returns the position of the column as part of the primary key,
 * or 0 if the column is not part of the key.
 */
static int get_pk_pos(const char *pkpos)
{
	if (*pkpos == 'N')
		return 0;
	return (int)str_to_long(pkpos);
}

/* tweak column_desc.c_len */
static void tweak_col_length(struct column_desc *col)
{
	switch (col->c_type) {
	case SMALLINT:
		col->c_len = 2;
		break;
	case INTEGER:
		col->c_len = 4;
		break;
	case BIGINT:
		col->c_len = 8;
		break;
	case DATE:
		col->c_len = 10;
		break;
	case TIME:
		col->c_len = 8;
		break;
	case TIMESTAMP:
		/* 20 is the number of characters before point */
		col->c_len += 20;
		break;
	default:
		break;
	}
}
