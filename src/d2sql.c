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

void data_record_to_sql(const unsigned char *d_rec_buff,
		        const struct column_desc *col_head)
{
	static char buff[D_REC_BUFF_SIZE];
	const unsigned char *walker;
	struct column_desc *col;
	size_t data_len;
	char *ascii_dec;

	col = col_head->next;
	memset(buff, 0x00, D_REC_BUFF_SIZE);

	while (col) {
		walker = d_rec_buff + IXFDCOLS_OFFSET + col->offset;
		if (col->nullable) {
			if (column_is_null(walker)) {
				printf("null");
				continue;
			} else {	/* bypass the null indicator */
				walker += 2;
			}
		}

		switch (col->type) {
		case CHAR:
			memcpy(buff, walker, col->length);
			printf("CHAR(%zd): %.*s ", col->length,
			       (int)col->length, buff);
			break;
		case VARCHAR:
			data_len = (size_t) parse_ixf_integer(walker, 2);
			memcpy(buff, walker + 2, data_len);
			printf("VARCHAR(%zd of %zd): %.*s ", data_len,
			       col->length, (int)data_len, buff);
			break;
		case SMALLINT:
			printf("SMALLINT: %hd ",
			       (short)parse_ixf_integer(walker,
							col->length));
			break;
		case INTEGER:
			printf("INTEGER: %ld ",
			       parse_ixf_integer(walker, col->length));
			break;
		case DECIMAL:
			ascii_dec =
			    decode_packed_decimal(walker, col->length);
			printf("DECIMAL: %s ", ascii_dec);
			free(ascii_dec);
			break;
		case TIMESTAMP:
			memcpy(buff, walker, col->length);
			printf("TIMESTAMP: %.*s ", (int)col->length, buff);
			break;
		default:
			err_exit("DB2 data type %d not implenmented",
				 col->type);
		}
		col = col->next;
	}

	puts("\n========= DATA RECORD END =========");
}
