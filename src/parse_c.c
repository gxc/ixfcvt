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

static void tweak_col_length(struct column_desc *col_desc);
static int get_pk_index(const char *buff);

/* parse a C record, store the info in a column_desc struct */
void parse_column_desc_record(const unsigned char *c_rec_buff,
			      struct column_desc *col_desc)
{
/* ignore IXFCDEFL, IXFCDEFV and IXFCKPOS now */
	static char buff[COL_ATTR_BUFF_SIZE];
	int col_name_len;

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, c_rec_buff + IXFCNAML_OFFSET, IXFCNAML_BYTES);
	col_name_len = str_to_long(buff);
	col_desc->name = alloc_buff(col_name_len + 1);
	memcpy(col_desc->name, c_rec_buff + IXFCNAME_OFFSET, col_name_len);
	col_desc->name[col_name_len] = '\0';

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, c_rec_buff + IXFCKPOS_OFFSET, IXFCKPOS_BYTES);
	col_desc->pk_index = get_pk_index(buff);

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, c_rec_buff + IXFCTYPE_OFFSET, IXFCTYPE_BYTES);
	col_desc->type = str_to_long(buff);
	memcpy(buff, c_rec_buff + IXFCLENG_OFFSET, IXFCLENG_BYTES);
	col_desc->length = str_to_long(buff);
	tweak_col_length(col_desc);

	memset(buff, 0x00, COL_ATTR_BUFF_SIZE);
	memcpy(buff, c_rec_buff + IXFCPOSN_OFFSET, IXFCPOSN_BYTES);
	/* the IXFDCOLS field of the D record starts at 1 (not 0) */
	col_desc->offset = str_to_long(buff) - 1;
	col_desc->nullable = c_rec_buff[IXFCNULL_OFFSET] == 'Y';
}

/*
 * Returns the position of the column as part of the primary key,
 * or 0 if the column is not part of the key.
 */
static int get_pk_index(const char *buff)
{
	if (*buff == 'N')
		return 0;
	return str_to_long(buff);
}

/* tweak column_desc.length */
static void tweak_col_length(struct column_desc *col_desc)
{
	switch (col_desc->type) {
	case SMALLINT:
		col_desc->length = 2U;
		break;
	case INTEGER:
		col_desc->length = 4U;
		break;
	case TIMESTAMP:
		/* 20 is the number of characters before point */
		col_desc->length += 20U;
		break;
	default:
		break;
	}
}
